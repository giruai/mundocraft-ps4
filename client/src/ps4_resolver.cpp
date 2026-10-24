// IPv4 getaddrinfo bridge for Luanti and libcurl on PS4.
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <cstddef>
#include <cstdint>

namespace {

// Keep the canaries adjacent to the native output object's exact bounds.
// The test build includes this source with PS4_RESOLVER_GUARD_ONLY defined.
template <typename T> struct GuardedNativeOutput {
	static_assert(alignof(T) <= alignof(uint32_t), "native output needs wider alignment");
	volatile uint32_t before[2] = {0x7a4d29b1u, 0xc65e083fu};
	T value{};
	volatile uint32_t after[2] = {0x3d91a76cu, 0xe0245bf8u};

	bool intact() const
	{
		return before[0] == 0x7a4d29b1u && before[1] == 0xc65e083fu &&
			after[0] == 0x3d91a76cu && after[1] == 0xe0245bf8u;
	}
};

} // namespace

#ifndef PS4_RESOLVER_GUARD_ONLY

#include <orbis/Net.h>
#include <orbis/NetCtl.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <climits>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <mutex>

#include "ps4_platform.h"

// socket.cpp owns the network service and pool. Luanti calls sockets_init()
// before httpfetch_init(), and httpfetch_cleanup() before sockets_cleanup().
extern "C" int32_t ps4_luanti_net_pool();

namespace {

std::mutex resolver_mutex;
std::atomic<unsigned> next_diagnostic_id{0};
std::atomic<unsigned> free_diagnostic_count{0};
constexpr unsigned MAX_DNS_DIAGNOSTIC_REQUESTS = 16;
constexpr unsigned MAX_DNS_DIAGNOSTIC_FREES = 32;

// ps4port::log uses fixed stack buffers. Keep every diagnostic transparent to
// the libc/native errno contract, including logs immediately after a failure.
template <typename... Args> void dns_log(const char *format, Args... args)
{
	const int saved_errno = errno;
	ps4port::log(format, args...);
	errno = saved_errno;
}

unsigned diagnostic_ticket(std::atomic<unsigned> &counter, unsigned limit)
{
	unsigned count = counter.load(std::memory_order_relaxed);
	while (count < limit) {
		if (counter.compare_exchange_weak(count, count + 1,
				std::memory_order_relaxed))
			return count + 1;
	}
	return 0;
}

using GuardedNetCtlInfo = GuardedNativeOutput<OrbisNetCtlInfo>;
using GuardedNetAddress = GuardedNativeOutput<OrbisNetInAddr>;
static_assert(offsetof(GuardedNetCtlInfo, value) == 2 * sizeof(uint32_t));
static_assert(offsetof(GuardedNetCtlInfo, after) ==
		offsetof(GuardedNetCtlInfo, value) + sizeof(OrbisNetCtlInfo));
static_assert(offsetof(GuardedNetAddress, value) == 2 * sizeof(uint32_t));
static_assert(offsetof(GuardedNetAddress, after) ==
		offsetof(GuardedNetAddress, value) + sizeof(OrbisNetInAddr));

[[noreturn]] void native_output_overrun(unsigned id, const char *stage,
		size_t output_size, int32_t status)
{
	dns_log("DNS_GUARD_FAIL id=%u stage=%s size=%llu rc=%d",
			id, stage, static_cast<unsigned long long>(output_size), status);
	std::abort();
}

enum class NetCtlOwnership { None, Borrowed, Owned };
NetCtlOwnership netctl_ownership = NetCtlOwnership::None;

bool ensure_netctl(unsigned id)
{
	if (netctl_ownership != NetCtlOwnership::None) {
		if (id)
			dns_log("DNS id=%u netctl reuse ownership=%u", id,
					static_cast<unsigned>(netctl_ownership));
		return true;
	}
	if (id)
		dns_log("DNS id=%u netctl_init begin", id);
	const int32_t status = sceNetCtlInit();
	if (id)
		dns_log("DNS id=%u netctl_init end rc=%d", id, status);
	if (status >= 0) {
		netctl_ownership = NetCtlOwnership::Owned;
		return true;
	}
	// The SDK has no trustworthy symbolic "already initialized" result.
	// Adopt an existing context only if a real query works.
	GuardedNetCtlInfo info;
	if (id)
		dns_log("DNS id=%u netctl_info begin size=%llu", id,
				static_cast<unsigned long long>(sizeof(info.value)));
	const int32_t info_status = sceNetCtlGetInfo(
			ORBIS_NET_CTL_INFO_IP_ADDRESS, &info.value);
	const bool guard_ok = info.intact();
	if (id)
		dns_log("DNS id=%u netctl_info end rc=%d guard=%u", id,
				info_status, unsigned(guard_ok));
	if (!guard_ok)
		native_output_overrun(id, "netctl_info", sizeof(info.value), info_status);
	if (info_status < 0)
		return false;
	netctl_ownership = NetCtlOwnership::Borrowed;
	return true;
}

int parse_service(const char *service, int flags, int &port, int &named_socktype)
{
	port = 0;
	named_socktype = 0;
	if (!service)
		return 0;
	if (!*service)
		return EAI_SERVICE;
	unsigned value = 0;
	bool numeric = true;
	for (const unsigned char *p = reinterpret_cast<const unsigned char *>(service); *p; ++p) {
		if (*p < '0' || *p > '9') {
			numeric = false;
			break;
		}
		if (value > (65535u - static_cast<unsigned>(*p - '0')) / 10u)
			return EAI_SERVICE;
		value = value * 10u + static_cast<unsigned>(*p - '0');
	}
	if (numeric) {
		port = static_cast<int>(value);
		return 0;
	}
	if (flags & AI_NUMERICSERV)
		return EAI_NONAME;
	if (std::strcmp(service, "http") == 0)
		port = 80;
	else if (std::strcmp(service, "https") == 0)
		port = 443;
	else
		return EAI_SERVICE;
	named_socktype = SOCK_STREAM;
	return 0;
}

int resolve_ipv4(const char *node, int flags, in_addr &address, unsigned id)
{
	if (!node) {
		if (id)
			dns_log("DNS id=%u address source=null", id);
		// Both constants are host-order values; convert before placing in sin_addr.
		address.s_addr = htonl((flags & AI_PASSIVE) ? INADDR_ANY : INADDR_LOOPBACK);
		return 0;
	}
	if (inet_pton(AF_INET, node, &address) == 1) {
		if (id)
			dns_log("DNS id=%u address source=numeric", id);
		return 0;
	}
	if (flags & AI_NUMERICHOST)
		return EAI_NONAME;

	std::lock_guard<std::mutex> lock(resolver_mutex);
	const int32_t pool = ps4_luanti_net_pool();
	if (id)
		dns_log("DNS id=%u native begin pool=%d", id, pool);
	if (pool < 0)
		return EAI_FAIL;
	if (!ensure_netctl(id))
		return EAI_AGAIN;
	if (id)
		dns_log("DNS id=%u resolver_create begin", id);
	const OrbisNetId resolver = sceNetResolverCreate("luanti-dns", pool, 0);
	if (id)
		dns_log("DNS id=%u resolver_create end rc=%d", id, resolver);
	if (resolver < 0)
		return EAI_FAIL;
	GuardedNetAddress result;
	if (id)
		dns_log("DNS id=%u resolver_start begin size=%llu timeout_us=2000000 retry=1",
				id, static_cast<unsigned long long>(sizeof(result.value)));
	const int32_t status = sceNetResolverStartNtoa(resolver, node, &result.value,
			2 * 1000 * 1000, 1, 0);
	const bool guard_ok = result.intact();
	if (id)
		dns_log("DNS id=%u resolver_start end rc=%d guard=%u", id,
				status, unsigned(guard_ok));
	if (!guard_ok)
		native_output_overrun(id, "resolver_start", sizeof(result.value), status);
	if (id)
		dns_log("DNS id=%u resolver_destroy begin", id);
	const int32_t destroy_status = sceNetResolverDestroy(resolver);
	if (id)
		dns_log("DNS id=%u resolver_destroy end rc=%d", id, destroy_status);
	if (status != 0)
		return EAI_AGAIN;
	address.s_addr = result.value.s_addr;
	return 0;
}

addrinfo *make_result(const in_addr &address, int port, int socktype,
		int protocol, const char *canonical, unsigned id)
{
	const size_t canonical_size = canonical ? std::strlen(canonical) + 1 : 0;
	const size_t total = sizeof(addrinfo) + sizeof(sockaddr_in) + canonical_size;
	if (id)
		dns_log("DNS id=%u result_alloc begin bytes=%llu socktype=%d",
				id, static_cast<unsigned long long>(total), socktype);
	auto *entry = static_cast<addrinfo *>(std::calloc(1, total));
	if (id)
		dns_log("DNS id=%u result_alloc end ptr=%p", id,
				static_cast<void *>(entry));
	if (!entry)
		return nullptr;
	auto *sin = reinterpret_cast<sockaddr_in *>(entry + 1);
	sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_port = htons(static_cast<uint16_t>(port));
	sin->sin_addr = address;
	entry->ai_family = AF_INET;
	entry->ai_socktype = socktype;
	entry->ai_protocol = protocol;
	entry->ai_addrlen = sizeof(*sin);
	entry->ai_addr = reinterpret_cast<sockaddr *>(sin);
	if (canonical) {
		auto *name = reinterpret_cast<char *>(sin + 1);
		std::memcpy(name, canonical, canonical_size);
		entry->ai_canonname = name;
	}
	if (id)
		dns_log("DNS id=%u result_ready ptr=%p", id,
				static_cast<void *>(entry));
	return entry;
}

} // namespace

extern "C" void ps4_luanti_resolver_cleanup()
{
	std::lock_guard<std::mutex> lock(resolver_mutex);
	if (netctl_ownership == NetCtlOwnership::Owned)
		sceNetCtlTerm();
	netctl_ownership = NetCtlOwnership::None;
}

extern "C" int getaddrinfo(const char *node, const char *service,
		const struct addrinfo *hints, struct addrinfo **result)
{
	const unsigned id = diagnostic_ticket(next_diagnostic_id,
			MAX_DNS_DIAGNOSTIC_REQUESTS);
	if (id)
		dns_log("DNS id=%u getaddrinfo begin node=%u service=%u", id,
				unsigned(node != nullptr), unsigned(service != nullptr));
	if (!result)
		return EAI_FAIL;
	*result = nullptr;
	if ((!node || !*node) && (!service || !*service))
		return EAI_NONAME;

	const int flags = hints ? hints->ai_flags : 0;
	const int supported_flags = AI_PASSIVE | AI_CANONNAME | AI_NUMERICHOST |
			AI_NUMERICSERV | AI_ADDRCONFIG;
	if (flags & ~supported_flags)
		return EAI_BADFLAGS;
	const int family = hints ? hints->ai_family : AF_UNSPEC;
	if (family != AF_UNSPEC && family != AF_INET)
		return EAI_FAMILY;
	const int requested_socktype = hints ? hints->ai_socktype : 0;
	const int requested_protocol = hints ? hints->ai_protocol : 0;
	if (requested_socktype && requested_socktype != SOCK_STREAM &&
			requested_socktype != SOCK_DGRAM)
		return EAI_SOCKTYPE;
	if (requested_protocol && requested_protocol != IPPROTO_TCP &&
			requested_protocol != IPPROTO_UDP)
		return EAI_SERVICE;
	if ((requested_socktype == SOCK_STREAM && requested_protocol == IPPROTO_UDP) ||
			(requested_socktype == SOCK_DGRAM && requested_protocol == IPPROTO_TCP))
		return EAI_SOCKTYPE;

	int port = 0;
	int named_socktype = 0;
	const int service_error = parse_service(service, flags, port, named_socktype);
	if (service_error)
		return service_error;
	if (named_socktype && (requested_socktype == SOCK_DGRAM ||
			requested_protocol == IPPROTO_UDP))
		return EAI_SERVICE;

	in_addr address{};
	const int resolve_error = resolve_ipv4(node && *node ? node : nullptr,
			flags, address, id);
	if (id)
		dns_log("DNS id=%u resolve end rc=%d", id, resolve_error);
	if (resolve_error)
		return resolve_error;

	const char *canonical = (flags & AI_CANONNAME) && node && *node ? node : nullptr;
	const bool stream = requested_socktype == SOCK_STREAM ||
			requested_protocol == IPPROTO_TCP || named_socktype == SOCK_STREAM;
	const bool datagram = requested_socktype == SOCK_DGRAM ||
			requested_protocol == IPPROTO_UDP;
	addrinfo *first = nullptr;
	addrinfo **tail = &first;
	if (stream || !datagram) {
		*tail = make_result(address, port, SOCK_STREAM, IPPROTO_TCP, canonical, id);
		if (!*tail)
			return EAI_MEMORY;
		tail = &(*tail)->ai_next;
		canonical = nullptr;
	}
	if (datagram || !stream) {
		*tail = make_result(address, port, SOCK_DGRAM, IPPROTO_UDP, canonical, id);
		if (!*tail) {
			freeaddrinfo(first);
			return EAI_MEMORY;
		}
	}
	*result = first;
	if (id)
		dns_log("DNS id=%u getaddrinfo end rc=0 head=%p", id,
				static_cast<void *>(first));
	return 0;
}

extern "C" void freeaddrinfo(struct addrinfo *result)
{
	while (result) {
		const unsigned free_id = diagnostic_ticket(free_diagnostic_count,
				MAX_DNS_DIAGNOSTIC_FREES);
		if (free_id)
			dns_log("DNS free=%u begin ptr=%p", free_id,
				static_cast<void *>(result));
		auto *next = result->ai_next;
		std::free(result);
		if (free_id)
			dns_log("DNS free=%u end", free_id);
		result = next;
	}
}

extern "C" const char *gai_strerror(int error)
{
	switch (error) {
	case 0: return "Success";
	case EAI_BADFLAGS: return "Invalid address hints";
	case EAI_NONAME: return "Name or service not known";
	case EAI_AGAIN: return "Temporary DNS resolution failure";
	case EAI_FAIL: return "DNS resolution failure";
	case EAI_FAMILY: return "Address family unsupported";
	case EAI_SOCKTYPE: return "Socket type unsupported";
	case EAI_SERVICE: return "Service unsupported";
	case EAI_MEMORY: return "Out of memory";
	case EAI_SYSTEM: return "System error";
	case EAI_OVERFLOW: return "Address buffer overflow";
	default: return "Unknown address error";
	}
}

#endif // !PS4_RESOLVER_GUARD_ONLY
