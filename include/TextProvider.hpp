#pragma once

#include <string>
#include <memory>
#include <optional>
#include <fstream>

using namespace std;

class PSLogger;

class TextProvider
{
public:
    TextProvider(shared_ptr<PSLogger> p_logger):m_logger(p_logger){};
    virtual ~TextProvider(){};

    virtual bool is_valid() {return !m_has_error;}
    virtual char advance() =0;
    virtual void reverse()=0;
    virtual char peek()=0;
    virtual char peek_next()=0;
    
    int get_current_char_index(){return m_current_char_idx;}
    char get_current_char(){return m_current_char;}

protected:
    static const string m_parser_text_provider_log_cat;
    shared_ptr<PSLogger> m_logger;
    bool m_has_error = false;
    int m_current_char_idx = -1;
    char m_current_char = '\0';
};

class TextProviderString : public TextProvider
{
public:
    TextProviderString(shared_ptr<PSLogger> p_logger, string p_text);
    virtual ~TextProviderString(){};

    virtual bool is_valid() override;
    virtual char advance() override;
    virtual void reverse() override;
    virtual char peek() override;
    virtual char peek_next() override;

protected:
    string m_text;
    optional<std::string::iterator> m_iterator;
};

class TextProviderFile : public TextProvider
{
public:
    TextProviderFile(shared_ptr<PSLogger> p_logger, string p_file_path);
    virtual ~TextProviderFile();

    virtual bool is_valid() override;
    virtual char advance() override;
    virtual void reverse() override;
    virtual char peek() override;
    virtual char peek_next() override;

protected:
    string m_file_path;
    ifstream m_file;
};