// SPDX-License-Identifier: MIT
#include "ps4_platform.h"
#include "ps4_heap_guard.h"
#include "launch_profile.h"
#include "ps4_sqlite.h"
#include "ps4_semaphore_check.h"
#include <vector>
#include <atomic>
#include <cwchar>
#include "log_internal.h"
#include <orbis_log.h>
#include <orbis/libkernel.h>
#include <orbis/SystemService.h>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <exception>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
extern "C" int ps4_libc_enable_malloc_locking(void);
extern int ps4_luanti_main(int,char**);
extern void ps4_luanti_shutdown();
extern "C" void ps4_thread_atexit_cleanup_main();
extern Logger g_logger;
class KernelLog final:public ICombinedLogOutput {
 void logRaw(LogLevel,std::string_view s) override {char b[1600];snprintf(b,sizeof(b),"%.*s",int(s.size()>1500?1500:s.size()),s.data());ps4port::log("%s",b);}
};
static void compat_log(const char *format,va_list args){char b[1500];vsnprintf(b,sizeof(b),format,args);ps4port::log("%s",b);}
static void copy_default(const char *destination,const char *source){
 struct stat st={};if(stat(destination,&st)==0)return;
 FILE *in=fopen(source,"rb");if(!in)return;FILE *out=fopen(destination,"wx");if(!out){fclose(in);return;}
 char buffer[4096];size_t n;while((n=fread(buffer,1,sizeof(buffer),in)))if(fwrite(buffer,1,n,out)!=n)break;
 fclose(out);fclose(in);
}
// Bounded, fixed-buffer phase output; no URL, response contents or credentials.
extern "C" void ps4_http_diag_stage(const char *stage, unsigned long long bytes)
{
 static std::atomic<unsigned> count{0};
 const unsigned event = count.fetch_add(1, std::memory_order_relaxed);
 if (event >= 64) {
  if (event == 64) ps4port::log("PS4_HTTP phase-budget reached limit=64");
  return;
 }
 ps4port::log("PS4_HTTP event=%u stage=%s bytes=%llu", event, stage, bytes);
 ps4_heap_guard_check(stage);
 ps4_heap_guard_report(stage);
}
int main(){
 orbis_set_log(compat_log);orbis_set_log_fatal(compat_log);
 ps4port::log("Luanti5.17.0 PS4 client alpha1 APP_VER=%s START",PS4_APP_VERSION);
 ps4port::log("malloc locking previous=%d",ps4_libc_enable_malloc_locking());
 ps4port::log("PS4_HEAP_GUARD enabled tail_bytes=16 capacity=%u",unsigned(ps4_heap_guard::kRegistryCapacity));
 ps4port::log("PS4_HEAP_REAL serialize=%d",int(ps4_heap_guard::kSerializeRealAllocator));
 ps4_heap_guard_check("startup");
 ps4_heap_guard_report("startup");
 ps4port::log("DIAG_CONFIG resolver_boundaries=guarded send_progress=bounded udp_results=bounded semaphore_backend=sceKernel");
 for(const char *path:{"/data/luanti","/data/luanti/cache","/data/luanti/tmp"}){errno=0;int rc=mkdir(path,0700);ps4port::log("DATA mkdir %s rc=%d errno=%d",path,rc,errno);}
 setenv("HOME","/data/luanti",1);setenv("TMPDIR","/data/luanti/tmp",1);
 setenv("MESA_SHADER_CACHE_DIR","/data/luanti/cache",1);setenv("ORBIS_SUBMIT_DONE","1",1);
 setenv("LUANTI_USER_PATH","/data/luanti",1);setenv("CURL_CA_BUNDLE","/app0/client/cacert.pem",1);
 copy_default("/data/luanti/minetest.conf","/app0/luanti/default.conf");
 copy_default("/data/luanti/launch.ini","/app0/luanti/launch.ini");
 KernelLog output;g_logger.addOutputMaxLevel(&output,LL_INFO);
 int result=1;
 unsigned char probe[32]={};bool entropy_ok=ps4port::secure_random(probe,sizeof(probe));for(volatile unsigned char &byte:probe)byte=0;
 std::wstring wide(L"Start\U0001F600");std::wstring wide_copy=wide;wide_copy.append(L" Done");
 bool wide_ok=sizeof(wchar_t)==4&&wcslen(L"Start")==5&&wide.size()==6&&wide_copy==L"Start\U0001F600 Done";
 ps4port::log("WCHAR_CHECK %s wchar_bytes=%u",wide_ok?"PASS":"FAIL",unsigned(sizeof(wchar_t)));
 if(!wide_ok)ps4port::log("FAIL wide-character runtime mismatch");
 else if(!entropy_ok)ps4port::log("FAIL native entropy unavailable");
 else{
  ps4port::log("ENTROPY_CHECK PASS");
  ps4port::LaunchProfile profile;std::string profile_text,error;
  FILE *file=fopen("/data/luanti/launch.ini","rb");if(file){char data[4097];size_t n=fread(data,1,sizeof(data),file);profile_text.assign(data,n);fclose(file);}
  bool sql_ok=ps4_sqlite_prepare();
  bool sem_ok=ps4_semaphore_selftest();
  bool profile_ok=ps4port::parse_profile(profile_text,profile,error);
  if(!profile_ok)ps4port::log("FAIL launch profile: %s",error.c_str());
  std::vector<std::string> args={"luanti","--config","/data/luanti/minetest.conf","--logfile","/data/luanti/debug.txt"};
  if(profile.lan){args.insert(args.end(),{"--go","--address",profile.address,"--port",std::to_string(profile.port),"--name",profile.name});}
  std::vector<char*> argv;for(auto &arg:args)argv.push_back(arg.data());argv.push_back(nullptr);
  ps4port::log("PS4_CONTROLS profile=native-gamepad pause=Options menu_back=Circle-or-Options");
  ps4port::log("PS4_CURSOR overlay=high-contrast place_trace=bounded");
  ps4port::log("START_MODE %s",profile.lan?"LAN direct":"menu");
  try{if(sql_ok&&sem_ok&&profile_ok)result=ps4_luanti_main(static_cast<int>(args.size()),argv.data());}catch(const std::exception &e){ps4port::log("FAIL client exception: %s",e.what());}catch(...){ps4port::log("FAIL unknown client exception");}
 }
 ps4_heap_guard_check("client-return");
 ps4_heap_guard_report("client-return");
 try{ps4_luanti_shutdown();}catch(...){ps4port::log("FAIL client shutdown exception");result=1;}
 ps4_thread_atexit_cleanup_main();
 ps4_heap_guard_check("shutdown");
 ps4_heap_guard_report("shutdown");
 g_logger.removeOutput(&output);
 ps4port::log("CLIENT_END result=%d cleanup=DONE; request Home",result);
 int rc=sceSystemServiceLoadExec("exit",nullptr);ps4port::log("native exit returned=%x",unsigned(rc));for(;;)sceKernelUsleep(1000000);
}
