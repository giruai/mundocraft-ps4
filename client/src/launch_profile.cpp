// SPDX-License-Identifier: MIT
#include "launch_profile.h"
#include <array>
#include <set>
namespace ps4port {
static std::string_view trim(std::string_view s){while(!s.empty()&&(s.front()==' '||s.front()=='\t'||s.front()=='\r'))s.remove_prefix(1);while(!s.empty()&&(s.back()==' '||s.back()=='\t'||s.back()=='\r'))s.remove_suffix(1);return s;}
static bool ipv4_lan(std::string_view s){std::array<unsigned,4>octets{};for(unsigned i=0;i<4;++i){auto dot=s.find('.');auto part=s.substr(0,dot);if(part.empty()||part.size()>3||(part.size()>1&&part.front()=='0'))return false;unsigned v=0;for(char c:part){if(c<'0'||c>'9')return false;v=v*10+unsigned(c-'0');}if(v>255)return false;octets[i]=v;if(i<3){if(dot==std::string_view::npos)return false;s.remove_prefix(dot+1);}else if(dot!=std::string_view::npos)return false;}return octets[0]==10||(octets[0]==192&&octets[1]==168)||(octets[0]==172&&octets[1]>=16&&octets[1]<=31);}
bool parse_profile(std::string_view text,LaunchProfile &result,std::string &error){
 if(text.size()>4096||text.find('\0')!=std::string_view::npos){error="profile size/encoding invalid";return false;}
 LaunchProfile parsed;std::set<std::string>seen;
 while(!text.empty()){
  auto end=text.find('\n');auto line=trim(text.substr(0,end));if(end==std::string_view::npos)text={};else text.remove_prefix(end+1);
  if(line.empty()||line.front()=='#')continue;auto eq=line.find('=');if(eq==std::string_view::npos){error="expected key=value";return false;}
  auto key=trim(line.substr(0,eq));auto value=trim(line.substr(eq+1));if(!seen.insert(std::string(key)).second){error="duplicate profile key";return false;}
  if(key=="mode"){if(value=="menu")parsed.lan=false;else if(value=="lan")parsed.lan=true;else{error="invalid mode";return false;}}
  else if(key=="address")parsed.address=value;
  else if(key=="name")parsed.name=value;
  else if(key=="port"){unsigned port=0;if(value.empty()){error="missing port";return false;}for(char c:value){if(c<'0'||c>'9'||port>6553){error="invalid port";return false;}port=port*10+unsigned(c-'0');}if(port<1||port>65535){error="port out of range";return false;}parsed.port=static_cast<uint16_t>(port);}
  else{error="unknown profile key";return false;}
 }
 if(parsed.name.empty()||parsed.name.size()>20){error="invalid player name length";return false;}
 for(char c:parsed.name)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-')){error="invalid player name";return false;}
 if(parsed.lan&&!ipv4_lan(parsed.address)){error="LAN mode needs a private IPv4 address";return false;}
 result=parsed;error.clear();return true;
}
}
