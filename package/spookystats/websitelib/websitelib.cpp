const char* homepage = 
    #include "homepage.html"
    ;
const char* system_telemetry = 
    #include "systemtelemetry.html"
    ;
extern "C" {
    const char* gethomepage(){
        return homepage;
    }
    const char* getsystemtelemetry(){
        return system_telemetry;
    }
}