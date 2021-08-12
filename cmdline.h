#ifndef CMDLINE_H
#define CMDLINE_H

#include <cxxabi.h>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cmd {
namespace detail {
    class CMDError : public std::exception {
    public:
        CMDError(const std::string& str)
        {
            msg += str;
        }
        CMDError() : CMDError("default error") {}

        CMDError(const CMDError&) = delete;
        CMDError(CMDError&&)      = delete;
        CMDError& operator=(const CMDError&) = delete;
        CMDError& operator=(CMDError&&) = delete;

        const char* what() const noexcept
        {
            return msg.c_str();
        }

    private:
        std::string msg = "CmdLine Error: ";
    };

    inline std::string Demangle(const std::string& name)
    {
        int   status = 0;
        char* ptr    = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
        if (status != 0) {
            if (ptr != nullptr) {
                free(ptr);
                ptr = nullptr;
            }
            throw CMDError("Fail to call __cxa_demangle during name Demangle "
                           "with status = " +
                           std::to_string(status));
        }
        std::string str(ptr);
        free(ptr);
        ptr = nullptr;
        return str;
    }

    template <typename T> std::string RealTypeName()
    {
        return Demangle(typeid(T).name());
    }

    template <> std::string RealTypeName<std::string>()
    {
        return "std::string";
    }

    template <typename T>
    inline std::string RealTypeNameOfClass = RealTypeName<T>();

    template <typename T> inline std::string RealTypeNameOfValue(T&& val)
    {
        return RealTypeName<std::remove_reference_t<decltype(val)>>();
    }

    template <typename Target, typename Source>
    Target LexicalCast(const Source& src)
    {
        if constexpr (std::is_same_v<Target, Source>) {
            return src;
        }
        else if constexpr (std::is_same_v<std::string, Target>) {
            std::stringstream ss;
            ss << std::boolalpha << src;
            return ss.str();
        }
        else if constexpr (std::is_same_v<std::string, Source>) {
            Target            ret;
            std::stringstream ss;
            ss << src;
            if (std::is_same_v<bool, Target> &&
                (src == "true" || src == "false")) {
                ss >> std::boolalpha >> ret;
                if (ss.fail()) {
                    throw CMDError("Bad cast from std::string " + src +
                                   " to bool");
                }
            }
            else {
                ss >> ret;
                if (!ss.eof() || ss.fail()) {
                    throw CMDError("Bad cast from std::string " + src + " to " +
                                   RealTypeNameOfClass<Target>);
                }
            }
            return ret;
        }
        else {
            throw CMDError("Unsupported cast from " +
                           RealTypeNameOfClass<Source> + " to " +
                           RealTypeNameOfClass<Target>);
        }
    }

    template <typename T>
    inline constexpr T CastFromString(const std::string& str)
    {
        return LexicalCast<T>(str);
    }

    template <typename T> inline std::string CastToString(const T& str)
    {
        return LexicalCast<std::string>(str);
    }

    template <typename T> class TypeCaster {
    public:
        virtual T operator()(const std::string& str) = 0;
        void      SetName(const std::string& str)
        {
            fullName = str;
        }
        std::string GetName()
        {
            return fullName;
        }
        void SetDesc(const std::string& str)
        {
            description = str;
        }
        std::string GetDesc()
        {
            return description;
        }

    private:
        std::string fullName{""};
        std::string description{""};
    };

    template <typename T> class StringCaster : public TypeCaster<T> {
    public:
        T operator()(const std::string& str) override
        {
            return CastFromString<T>(str);
        }
    };

    template <typename T> class ChoiceCaster : public TypeCaster<T> {
    public:
        ChoiceCaster(std::initializer_list<T> list)
        {
            std::stringstream ss;
            ss << "[";
            for (auto it = list.begin(); it != list.end();) {
                choiceList.emplace(*it);
                ss << CastToString(*it);
                if (++it != list.end()) {
                    ss << ", ";
                }
            }
            ss << "]";
            this->SetDesc(ss.str());
        }
        ~ChoiceCaster() = default;
        T operator()(const std::string& str) override
        {
            T ret = CastFromString<T>(str);
            if (choiceList.count(ret) == 0) {
                throw CMDError("Argument: " + this->GetName() +
                               " should be in " + this->GetDesc() + ", got " +
                               str + " indeed");
            }

            return ret;
        }

    private:
        std::unordered_set<T> choiceList{};
    };

    class Argument {
    public:
        virtual std::string FullName()                   = 0;
        virtual std::string ShortName()                  = 0;
        virtual std::string Description()                = 0;
        virtual std::string Usage()                      = 0;
        virtual bool        Required()                   = 0;
        virtual bool        IsGood()                     = 0;
        virtual bool        Setted()                     = 0;
        virtual void        Read(const std::string& str) = 0;
    };

    template <typename T> class BasicArgument : public Argument {
    public:
        BasicArgument(const std::string& fullName,
                      const std::string& shortName,
                      const std::string& description,
                      const bool         required,
                      const T&           defaultValue)
            : fullName_(fullName), shortName_(shortName),
              description_(description), required_(required),
              defaultValue_(defaultValue)
        {
            std::stringstream ss;
            ss << "\tfullName: " << fullName << ",\n"
               << "\tshortName: " << shortName << ",\n"
               << "\tdescription: " << description << ",\n"
               << "\trequired: " << std::boolalpha << required << ",\n";
            if constexpr (std::is_same_v<std::string, T>) {
                if (defaultValue.size() > 0) {
                    actualValue_ = defaultValue;
                    hasDefault_  = true;
                    ss << "\tdefaultValue: " << defaultValue << ",\n";
                }
            }
            else {
                if (defaultValue) {
                    actualValue_ = defaultValue;
                    hasDefault_  = true;
                    ss << "\tdefaultValue: " << defaultValue << ",\n";
                }
            }
            usage_ = ss.str();
        }
        ~BasicArgument() = default;

        virtual std::string FullName() override
        {
            return fullName_;
        }
        virtual std::string ShortName() override
        {
            return shortName_;
        }
        virtual std::string Description() override
        {
            return description_;
        }
        virtual std::string Usage() override
        {
            return usage_;
        }
        virtual bool Required() override
        {
            return required_;
        }
        virtual bool IsGood() override
        {
            bool ret = false;
            if (setted_ || hasDefault_ || (!required_ && hasDefault_)) {
                ret = true;
            }
            else {
                throw CMDError(
                    "Argument: " + fullName_ +
                    " is not required, but has no default value yet");
            }
            return ret;
        }
        virtual bool Setted() override
        {
            return setted_;
        }
        virtual void Read(const std::string& str) override
        {
            actualValue_ = stringCaster(str);
            setted_      = true;
        }

        T ActualValue()
        {
            return actualValue_;
        }

    protected:
        std::string     fullName_{""};
        std::string     shortName_{""};
        std::string     description_{""};
        std::string     usage_{""};
        bool            required_{false};
        bool            setted_{false};
        bool            hasDefault_{false};
        T               defaultValue_{};
        T               actualValue_{};
        StringCaster<T> stringCaster{};
    };

    template <typename T> class ChoiceArgument : public BasicArgument<T> {
    public:
        ChoiceArgument(const std::string&              fullName,
                       const std::string&              shortName,
                       const std::string&              description,
                       const bool                      required,
                       const T&                        defaultValue,
                       const std::initializer_list<T>& list)
            : BasicArgument<T>(fullName,
                               shortName,
                               description,
                               required,
                               defaultValue)
        {
            choiceCaster = ChoiceCaster<T>(list);
            choiceCaster.SetName(fullName);
            this->usage_ = this->usage_ +
                           "\tchoice from: " + choiceCaster.GetDesc() + "\n";
        }
        ~ChoiceArgument() = default;

        virtual void Read(const std::string& str) override
        {
            this->actualValue_ = choiceCaster(str);
            this->setted_      = true;
        }

    private:
        ChoiceCaster<T> choiceCaster{};
    };

    std::vector<std::string> StringSplit(const std::string& src,
                                         const std::string& pattern = "=")
    {
        std::vector<std::string> ret;
        std::string              str = src;
        if (str.empty()) {
            return ret;
        }
        std::string::size_type pos;
        str += pattern;
        auto size = str.size();
        for (size_t i = 0; i < size; i++) {
            pos = str.find(pattern, i);
            if (pos < size) {
                auto s = str.substr(i, pos - i);
                ret.push_back(std::move(s));
                i = pos + pattern.size() - 1;
            }
        }
        return ret;
    }
}  // namespace detail

class Parser {
public:
    Parser() = default;
    ~Parser()
    {
        fullArguments.clear();
        shortToFull.clear();
    }

    template <typename T>
    void Add(const std::string&              fullName,
             const std::string&              shortName    = "",
             const std::string&              description  = "",
             const bool                      required     = false,
             const T&                        defaultValue = T{},
             const std::initializer_list<T>& list         = {})
    {
        CheckExist(fullName, shortName);
        std::shared_ptr<detail::Argument> arg;
        if (list.size() == 0) {
            arg = std::make_shared<detail::BasicArgument<T>>(
                fullName, shortName, description, required, defaultValue);
        }
        else {
            arg = std::make_shared<detail::ChoiceArgument<T>>(
                fullName, shortName, description, required, defaultValue, list);
        }
        AddArgument(fullName, shortName, arg);
    }

    void Parse(int argc, char* argv[])
    {
        progName = argv[0];
        if (argc < 2) {
            std::cout << Usage() << std::endl;
            exit(1);
        }
        for (int i = 1; i < argc; i++) {
            std::string str(argv[i]);
            if (str.find("=") == std::string::npos) {
                throw detail::CMDError(
                    "Argument should be in the form of "
                    "'--fullName=value' or '-shorName=value', got " +
                    str + " indeed");
            }
            auto splited = detail::StringSplit(str);
            auto name    = splited[0];
            auto value   = splited[1];
            if (name.find("--") != std::string::npos) {
                ReadWithFullName(name, value);
            }
            else if (name.find("-") != std::string::npos) {
                auto it = shortToFull.find(name);
                if (it == shortToFull.end()) {
                    throw detail::CMDError("Not exist argument: " + name);
                }
                ReadWithFullName(it->second, value);
            }
            else {
                throw detail::CMDError(
                    "Argument should be in the form of "
                    "'--fullName=value' or '-shorName=value', got " +
                    str + " indeed");
            }
        }

        for (auto it = fullArguments.begin(); it != fullArguments.end(); ++it) {
            if (!it->second->IsGood()) {
                throw detail::CMDError("Required argument: " + it->first +
                                       " is not setted");
            }
        }
    }

    template <typename T> T Get(const std::string& name)
    {
        if (name.find("--") != std::string::npos) {
            return GetWithFullName<T>(name);
        }
        else if (name.find("-") != std::string::npos) {
            auto it = shortToFull.find(name);
            if (it == shortToFull.end()) {
                throw detail::CMDError("Not exist argument: " + name);
            }
            return GetWithFullName<T>(it->second);
        }
        else {
            throw detail::CMDError("Not exist argument: " + name);
        }
    }

private:
    std::string Usage() const
    {
        std::stringstream ss;
        ss << "Usage: " << progName << " ";
        for (auto it = fullArguments.begin(); it != fullArguments.end(); ++it) {
            if (it->second->Required()) {
                ss << it->first << " ";
            }
        }
        ss << "\n[options]..." << std::endl;
        for (auto it = fullArguments.begin(); it != fullArguments.end(); ++it) {
            ss << it->second->Usage() << std::endl;
        }
        return ss.str();
    }

    template <typename T> T GetWithFullName(const std::string& fullName)
    {
        auto it = fullArguments.find(fullName);
        if (it == fullArguments.end()) {
            throw detail::CMDError("Not exist argument: " + fullName);
        }
        auto innerArg =
            reinterpret_cast<detail::BasicArgument<T>*>(it->second.get());
        return innerArg->ActualValue();
    }

    void ReadWithFullName(const std::string& fullName, const std::string& val)
    {
        auto it = fullArguments.find(fullName);
        if (it == fullArguments.end()) {
            throw detail::CMDError("Not exist argument: " + fullName);
        }
        if (it->second->Setted()) {
            throw detail::CMDError("Argument " + fullName + " has been setted");
        }
        it->second->Read(val);
    }

    void CheckExist(const std::string& fullName, const std::string& shortName)
    {
        if (fullArguments.count(fullName)) {
            throw detail::CMDError("Multiple definition for " + fullName);
        }
        if (std::size(shortName) > 0 && shortToFull.count(shortName)) {
            throw detail::CMDError("Multiple definition for " + shortName);
        }
    }

    void AddArgument(const std::string&                       fullName,
                     const std::string&                       shortName,
                     const std::shared_ptr<detail::Argument>& ptr)
    {
        fullArguments.insert(std::make_pair(fullName, ptr));
        if (std::size(shortName) > 0) {
            shortToFull.insert(std::make_pair(shortName, fullName));
        }
    }

private:
    std::unordered_map<std::string, std::shared_ptr<detail::Argument>>
                                                 fullArguments{};
    std::unordered_map<std::string, std::string> shortToFull{};
    std::string                                  progName{""};
};

}  // namespace cmd

#endif