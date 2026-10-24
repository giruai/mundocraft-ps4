// SPDX-License-Identifier: MIT
#include "ps4_platform.h"
#include <orbis/libkernel.h>
#include <orbis/Sysmodule.h>
#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <mutex>
namespace {
using RandomFn=int32_t (*)(void*,size_t);
RandomFn random_bytes=nullptr;
std::once_flag random_once;
void resolve_random(){
 int32_t module_status=sceSysmoduleLoadModule(ORBIS_SYSMODULE_RANDOM);
 ps4port::log("RNG sysmodule load rc=%x",unsigned(module_status));
 // Fixed system module and export only. Never load a user-controlled module path.
 int32_t handle=static_cast<int32_t>(sceKernelLoadStartModule("/system/common/lib/libSceRandom.sprx",0,nullptr,0,nullptr,nullptr));
 ps4port::log("RNG path load rc=%x",unsigned(handle));
 void *symbol=nullptr;
 if(handle>=0&&sceKernelDlsym(handle,"sceRandomGetRandomNumber",&symbol)>=0&&symbol){random_bytes=reinterpret_cast<RandomFn>(symbol);return;}
 OrbisKernelModule modules[256]={};size_t count=0;
 int32_t list_status=sceKernelGetModuleList(modules,256,&count);
 ps4port::log("RNG module scan rc=%x count=%u",unsigned(list_status),unsigned(count));
 if(list_status<0||count>256)return;
 RandomFn found=nullptr;
 for(size_t i=0;i<count;++i){symbol=nullptr;if(sceKernelDlsym(modules[i],"sceRandomGetRandomNumber",&symbol)>=0&&symbol){auto fn=reinterpret_cast<RandomFn>(symbol);if(found&&found!=fn)return;found=fn;}}
 random_bytes=found;
 ps4port::log("RNG export %s",found?"resolved":"unavailable");
}
}
namespace ps4port {
void log(const char *format,...){char message[1600];va_list args;va_start(args,format);vsnprintf(message,sizeof(message),format,args);va_end(args);char line[1660];snprintf(line,sizeof(line),"[LUNT00001] %s\n",message);sceKernelDebugOutText(0,line);}
bool prepare_random(){std::call_once(random_once,resolve_random);return random_bytes!=nullptr;}
bool secure_random(void *output,size_t size){if((!output&&size)||!prepare_random())return false;auto *bytes=static_cast<unsigned char *>(output);while(size){size_t part=size>64?64:size;int32_t rc=random_bytes(bytes,part);if(rc!=0){log("RNG generate rc=%x",unsigned(rc));return false;}bytes+=part;size-=part;}return true;}
}
// Mbed TLS hardware entropy contract; no predictable fallback or /dev/urandom.
extern "C" int mbedtls_hardware_poll(void*,unsigned char *output,size_t size,size_t *written){if(!written)return -0x003c;*written=0;if(!ps4port::secure_random(output,size))return -0x003c;*written=size;return 0;}
