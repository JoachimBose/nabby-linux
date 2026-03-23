const char* homepage = 
    #include "homepage.html"
    ;
extern "C" {
    const char* gethomepage(){
        return homepage;
    }
}