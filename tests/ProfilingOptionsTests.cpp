#include "ProfilingOptions.h"
#include <cstdio>
#include <initializer_list>

#define CHECK(x) do{if(!(x)){printf("FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    // A private variable avoids changing a running launcher's/game's options.
    auto name=L"G3VR_TEST_PRIVATE_PROFILE_OPT_IN";
    CHECK(SetEnvironmentVariableW(name,nullptr));
    CHECK(!diagnostic::performance::ProfilingOptIn(name));
    for(auto value:{L"0",L"true",L"01",L"10",L"1 ",L" 1",L"111111111111111111111111111111"}){
        CHECK(SetEnvironmentVariableW(name,value));
        CHECK(!diagnostic::performance::ProfilingOptIn(name));
    }
    CHECK(SetEnvironmentVariableW(name,L"1"));
    CHECK(diagnostic::performance::ProfilingOptIn(name));
    CHECK(SetEnvironmentVariableW(name,L""));
    CHECK(!diagnostic::performance::ProfilingOptIn(name));
    CHECK(SetEnvironmentVariableW(name,nullptr));
    puts("PASS explicit profiling opt-in; absent, malformed and truncated values stay off");
}
