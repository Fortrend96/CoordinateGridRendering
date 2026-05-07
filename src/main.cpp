#include "Application.h"

#include <iostream>
#include <exception>

int main()
{
    try
    {
        CApplication application;
        return application.Run();
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}