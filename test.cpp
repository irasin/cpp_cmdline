#include <cmdline.h>
#include <iostream>

int main(int argc, char* argv[])
{
    cmd::Parser p;

    p.Add<std::string>("--stringArg", "-s", "this is a string argument", true);
    p.Add<int>("--intArg", "-i", "this is a int argument with default value 1",
               true, 1);
    p.Add<float>(
        "--floatArg", "-f",
        "this is a float argument with default value 0.1, and should be "
        "in [0.1, 0.01, 0.001]",
        false, 0.1, {0.1, 0.01, 0.001});

    p.Add<bool>("--boolArg", "",
                "this is a bool argument with default value true", false, true);
    p.Parse(argc, argv);

    auto stringArg = p.Get<std::string>("--stringArg");
    auto intArg    = p.Get<int>("-i");
    auto floatArg  = p.Get<float>("-f");
    auto boolArg   = p.Get<bool>("--boolArg");
    std::cout << "stringArg: " << stringArg << ", intArg: " << intArg
              << ", floatArg: " << floatArg << std::boolalpha
              << ", boolArg: " << boolArg << std::endl;

    return 0;
}