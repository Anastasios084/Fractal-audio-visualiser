#include "audioAnalyzer.h"
#include <iostream>

using namespace std;

int main(){
    audioAnalyzer anal;
    if(!anal.init()){
        cout << "Failed to initialize class" << endl;
    }
    anal.startSession(30);
    
    return 0;
}