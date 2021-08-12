# C++ cmdline parser 

A c++ cmdline argument parser tool based on c++17, presented as a single headfile.

## Usage

It is easy to use the `cmd::Parser` class within three steps;

1. Use `Add<T>()` to define argument, we support the form as below. In the choice argument mode, which means the value should be one of some values, you need provide a choice list and also a default value.

   ```c++
   template <typename T>
   void Add(const std::string&              fullName,
            const std::string&              shortName    = "",
            const std::string&              description  = "",
            const bool                      required     = false,
            const T&                        defaultValue = T{},
            const std::initializer_list<T>& list         = {})
   ```

   Here is some examples.

   ```c++
   p.Add<std::string>("--fileName", "-f", "this is a file name", true);
   p.Add<int>("--epoch", "-e", "this is epoch number", true, 3);
   p.Add<float>("--learningRate", "-lr", "this is learning rate", false, 0.1, {0.1, 0.01, 0.001});
   ```

   

2. Use `Parse(argc, argv)`to parse the cmdline argument

3. Use `Get<T>()` to get the value.Both fullName and shortName are supported here.

   ```c++
   auto fileName = p.Get<std::string>("-f")
   auto epoch = p.Get<int>("--epoch");
   auto lr = p.Get<float>("-lr");
   ```

4. Running without arguments will show the usage BTW.

## Full example

1. write the source file.

```c++
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
```

2. compile with `-std=c++17 -I{path to cmdline.h dir}`

   ```bash
   g++ test.cpp -o cmd -std=c++2a -I/Users/irasin/Desktop/cpp_argument_parser 
   ```

   

3. run

   * run without argument to show usage

     ```bash
     ./cmd 
     
     Usage: ./cmd --intArg --stringArg 
     [options]...
     	fullName: --floatArg,
     	shortName: -f,
     	description: this is a float argument with default value 0.1, and should be in [0.1, 0.01, 0.001],
     	required: false,
     	defaultValue: 0.1,
     	choice from: [0.1, 0.01, 0.001]
     
     	fullName: --boolArg,
     	shortName: ,
     	description: this is a bool argument with default value true,
     	required: false,
     	defaultValue: true,
     
     	fullName: --intArg,
     	shortName: -i,
     	description: this is a int argument with default value 1,
     	required: true,
     	defaultValue: 1,
     
     	fullName: --stringArg,
     	shortName: -s,
     	description: this is a string argument,
     	required: true,
     
     
     ```

   * run with argument

     ```bash
     ./cmd --intArg=1 --stringArg=hello -f=0.01 --boolArg=false
     stringArg: hello, intArg: 1, floatArg: 0.01, boolArg: false
     ```

     

