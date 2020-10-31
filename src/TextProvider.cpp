
#include "TextProvider.hpp"
#include "PSLogger.hpp"

const string TextProvider::m_parser_text_provider_log_cat = "parser_text_provider";

TextProviderString::TextProviderString(shared_ptr<PSLogger> p_logger, string p_text)
:TextProvider(p_logger), m_text(p_text)
{
	//todo add some kind of check in case the logger isn't valid

	if(m_text.empty())
	{
		m_logger->log(PSLogger::LogType::Error, m_parser_text_provider_log_cat, "text provided is empty. Parsing will fail.");
		m_has_error = true;
	}
	else
	{
		m_current_char_idx = 0;
	}
}

bool TextProviderString::is_valid()
{
	return TextProvider::is_valid() && m_iterator != m_text.end();
}

char TextProviderString::advance()
{
	//todo this is probably the wrong way to deal with the fact that contrary to the "get" method on the file, the iterator already points to a valid char if the text is not empty but I'm too tired so this'll work for now.
	//todo it's ugly though and it makes "peek" unusable before "advance". Also, using "reverse" until the beggining of the string would mess it all
	bool avoid_iterator_increment = false;

	if(!m_iterator.has_value())
	{
		m_iterator = m_text.begin();
		avoid_iterator_increment = true;
	}

	if(is_valid())
	{
		++m_current_char_idx;

		if(!avoid_iterator_increment)
		{
			++m_iterator.value();
		}

		if(m_iterator.value() != m_text.end())
		{
			m_current_char = *m_iterator.value();
		}
		else
		{
			m_current_char = '\0';
		}

		if( static_cast<unsigned char>(m_current_char) > 127)
		{
			m_logger->log(PSLogger::LogType::Error, m_parser_text_provider_log_cat,
			"detected non ascii character ( n : "+to_string(m_current_char_idx)+". this interpreter does not yet support unicode files, sorry :/");
			//todo add a line counter in this class to make these function return the correct line
			m_has_error = true;
		}
	}
	else
	{
		m_current_char = '\0';
	}
	return m_current_char;
}

void TextProviderString::reverse()
{
	if(is_valid() && m_iterator.has_value())
	{
		--m_current_char_idx;
		--m_iterator.value();
		//todo the m_current_char will probably be incorrect
	}
}
char TextProviderString::peek()
{
	if( !m_iterator.has_value())
	{
		m_logger->log(PSLogger::LogType::Error, m_parser_text_provider_log_cat,
			"Cannot yet use peek before a first advance with the textProviderString, sorry :/");
		m_has_error = true;
		return '\0';
	}

	int peeked_at_index = (int)(m_iterator.value() - m_text.begin()) + 1;

	if(is_valid() && peeked_at_index < m_text.size())
	{
		return m_text.at(peeked_at_index);
	}
	else
	{
		return '\0';
	}
}
char TextProviderString::peek_next()
{
	if( !m_iterator.has_value())
	{
		m_logger->log(PSLogger::LogType::Error, m_parser_text_provider_log_cat,
			"Cannot yet use peek before a first advance with the textProviderString, sorry :/");
		m_has_error = true;
		return '\0';
	}

	int peeked_at_index = (int)(m_iterator.value() - m_text.begin()) + 2;

	if(is_valid() && peeked_at_index < m_text.size())
	{
		return m_text.at(peeked_at_index);
	}
	else
	{
		return '\0';
	}
}

TextProviderFile::TextProviderFile(shared_ptr<PSLogger> p_logger, string p_file_path)
:TextProvider(p_logger), m_file_path(p_file_path)
{
	//todo add some kind of check in case the logger isn't valid

	m_file.open(m_file_path);

	if(!m_file.is_open())
	{
		m_logger->log(PSLogger::LogType::Error, m_parser_text_provider_log_cat, "couldn't open file : "+ m_file_path + ". Parsing will fail.");
		m_has_error = true;
	}
	else
	{
		m_current_char_idx = 0;
	}
}

TextProviderFile::~TextProviderFile()
{
	if(m_file.is_open())
	{
		m_file.close();
	}
}

bool TextProviderFile::is_valid()
{
	return TextProvider::is_valid() && m_file.is_open() && !m_file.eof();
}

char TextProviderFile::advance()
{
	if(is_valid())
	{
		++m_current_char_idx;
		m_file.get(m_current_char);

		if( static_cast<unsigned char>(m_current_char) > 127)
		{
			m_logger->log(PSLogger::LogType::Error, m_parser_text_provider_log_cat,
			"detected non ascii character. this interpreter does not yet support unicode files, sorry :/");
			//todo add a line counter in this class to make these function return the correct line
			m_has_error = true;
		}
	}
	else
	{
		m_current_char = '\0';
	}
	return m_current_char;
}

void TextProviderFile::reverse()
{
	if(is_valid())
	{
		--m_current_char_idx;
		m_file.unget();
		//todo the m_current_char will probably be incorrect
	}
}
char TextProviderFile::peek()
{
	if(is_valid())
	{
		return (char)m_file.peek();
	}
	else
	{
		return '\0';
	}
}
char TextProviderFile::peek_next()
{
	if(is_valid())
	{
		//todo make this function secure. it may crash if called close to eof ?
		char result;
		m_file.get(result);
		result = (char)m_file.peek();
		m_file.unget();
		return result;
	}
	else
	{
		return '\0';
	}
}
