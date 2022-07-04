#include "config/configfile.h"
#include "log/log.h"

#include <fstream>
#include <algorithm>
#include <iomanip>

using namespace kmicki::log;

namespace kmicki::config
{
    ConfigFile::ConfigFile(std::string const& _filePath)
    {
        filePath = _filePath;
        
        { LogF(LogLevelTrace) << "ConfigFile: Initialize with filepath: " << _filePath; }
    }

    const char ConfigFile::cSeparator = ':';

    bool ConfigFile::LoadConfig(std::vector<std::unique_ptr<ConfigItemBase>> & configuration) const
    {
        static const int cBufLen = 500;
        static const char cCommentPrefix = '#';

        { LogF(LogLevelTrace) << "ConfigFile::LoadConfig: Loading configuration from file: " << filePath; }
        std::ifstream file(filePath);
        if(file.fail())
        {
            Log("ConfigFile::LoadConfig: File open failed.",LogLevelDebug);
            return false;
        }

        char buf[cBufLen];

        int line = 0;

        std::string precedingComment = "";

        while(!file.eof())
        {
            ++line;
            file.getline(buf,cBufLen);
            std::string str(buf);

            // detect preceding comment
            if(*(str.begin()) == cCommentPrefix)
            {
                str.erase(str.begin());

                auto c = str.begin();
                while(std::isspace(*c))
                    c = str.erase(c);

                precedingComment.append(str);
                precedingComment.push_back('\n');

                { LogF(LogLevelTrace) << "ConfigFile::LoadConfig: Line " << line << " - comment: " << str; }
                continue;
            }

            std::string precedingCommentAll = "";
            precedingCommentAll.swap(precedingComment);
            std::string inlineComment = "";

            // remove white space and comment
            auto c = str.begin();
            while(c != str.end())
                if(std::isspace(*c))
                    c = str.erase(c);
                else if(*c == cCommentPrefix)
                {
                    auto ch = c+1;
                    while(std::isspace(*ch))
                        ++ch;
                    inlineComment = std::string(ch,str.end());
                    c = str.erase(c,str.end());
                }
                else
                    ++c;

            if(str.empty())
            {
                { LogF(LogLevelTrace) << "ConfigFile::LoadConfig: Line " << line << " - empty or comment"; }
                continue;
            }

            auto pos = str.find(cSeparator);
            if(pos == std::string::npos)
            {
                { LogF(LogLevelDebug) << "ConfigFile::LoadConfig: Line " << line << " - invalid, no separator found (" << cSeparator << ")"; }
                continue;
            }

            if(pos == 0)
            {
                { LogF(LogLevelDebug) << "ConfigFile::LoadConfig: Line " << line << " - invalid, no name found"; }
                continue;                
            }

            auto name = str.substr(0,pos);
            auto item = std::find_if(configuration.begin(),configuration.end(),
                            [&](std::unique_ptr<ConfigItemBase> & x)
                            { 
                                return x->Name == name;
                            }
                        );

            std::string value("");
            if(pos < str.length()-1)
                value = str.substr(pos+1);

            if(item == configuration.end())
            {
                { LogF(LogLevelTrace) << "ConfigFile::LoadConfig: Line " << line << " - new item, Name=" << name << " Value=" << value; }
                configuration.emplace_back(new ConfigItem<std::string>(name,value));
                continue;
            }

            std::string msg;
            if((*item)->Update(value,msg))
                { LogF(LogLevelTrace) << "ConfigFile::LoadConfig: Line " << line << " - " << msg; }
            else
                { LogF(LogLevelDebug) << "ConfigFile::LoadConfig: Line " << line << " - " << msg; }
        }

        Log("ConfigFile::LoadConfig: Configuration loaded.",LogLevelDebug);
        return true;
    }

    bool ConfigFile::SaveConfig(std::vector<std::unique_ptr<ConfigItemBase>> const& configuration,bool pretty) const
    {
        { LogF(LogLevelTrace) << "ConfigFile::SaveConfig: Saving configuration to file: " << filePath; }
        std::ofstream file(filePath,std::ios_base::trunc);
        if(file.fail())
        {
            Log("ConfigFile::SaveConfig: File open failed.",LogLevelDebug);
            return false;
        }
        
        int max = 0;

        if(pretty)
        {
            // Find longest name
            for(auto& item : configuration)
                if(item->Name.length() > max)
                    max = item->Name.length();
            { LogF(LogLevelTrace) << "ConfigFile::SaveConfig: Pretty layout enabled. Found maximum name length: " << max; }
        }

        int line = 0;
        for(auto& item : configuration)
        {
            std::string value = item->ValToString();
            file << std::setw(max) << std::left << item->Name << ' ' << cSeparator << ' ' << value << std::endl;
            { LogF(LogLevelTrace) << "ConfigFile::SaveConfig: Line " << ++line << " - saved item, Name=" << item->Name << " Value=" << value; }
        }

        Log("ConfigFile::SaveConfig: Configuration saved.",LogLevelDebug);

        return true;
    }
}