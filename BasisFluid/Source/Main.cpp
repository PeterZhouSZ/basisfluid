
#include "Application.h"
#include <iostream>

using namespace std;

Application* app = nullptr;

int main()
{
    app = new Application();
    app->Run();

    system("pause");
    return 0;

}
    
    

    //Window* pWindow = new Window(width, height, title, hints, share);
    //sWindowPointersMap.insert(pair<GLFWwindow*,Window*>(pWindow->mpGlfwWindow, pWindow));



    //clog << "Starting main loop." << endl;
    //glfwSwapInterval(0);
    //while(true) {
    //
    //    glfwPollEvents();

    //    if(elapsedTime() >= mUnwaitTime) {

    //        mainLoop();
    //        ++mNbFrames;

    //        // refresh windows
    //        for(auto& iWindowPair : sWindowPointersMap)
    //        {
    //            iWindowPair.second->swapBuffers();
    //        }
    //    }

    //    double endTime = elapsedTime();
    //    computeTimePreviousFrame = endTime - begTime;
    //    if(computeTimePreviousFrame != 0) {
    //        fpsPreviousFrame = 1.0/computeTimePreviousFrame;
    //    } else {
    //        fpsPreviousFrame = 99999;
    //    }
    //    out_int_computeTimePreviousFrame.dirtyConnections();
    //    out_int_fpsPreviousFrame.dirtyConnections();

    //}
    //glfwTerminate();


