#include "Parser.hpp"


#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>

#include <set>

using namespace std;

map<string, FileSection, ci_less> to_file_section = {
	{"None", FileSection::None},
	{"Prelude", FileSection::Prelude},
	{"Objects", FileSection::Objects},
	{"Legend", FileSection::Legend},
	{"Sounds", FileSection::Sounds},
	{"CollisionLayers", FileSection::CollisionLayers},
	{"Rules", FileSection::Rules},
	{"WinConditions", FileSection::WinConditions},
	{"Levels", FileSection::Levels},
};

const string Parser::m_parser_log_cat = "parser";

std::optional<ParsedGame> Parser::parse_from_file(string p_file_path, shared_ptr<PSLogger> p_logger)
{
	unique_ptr<TextProvider> txt_provider(new TextProviderFile(p_logger,p_file_path));
	return parse_from_text_provider(move(txt_provider),p_logger);
}

std::optional<ParsedGame> Parser::parse_from_string(string p_text, shared_ptr<PSLogger> p_logger)
{
	unique_ptr<TextProvider> txt_provider(new TextProviderString(p_logger,p_text));
	return parse_from_text_provider(move(txt_provider),p_logger);
}

std::optional<ParsedGame> Parser::parse_from_text_provider(unique_ptr<TextProvider> p_text_provider, shared_ptr<PSLogger> p_logger)
{
	Parser parser(move(p_text_provider),p_logger);
	return parser.parse_file();
}

Parser::Parser(unique_ptr<TextProvider> p_text_provider, shared_ptr<PSLogger> p_logger)
: m_logger(p_logger)
{
	m_text_provider = std::move(p_text_provider);
	if(m_logger == nullptr)
	{
		cout << "A Logger ptr was not passed to the parser constructor, construction a default one\n";
		m_logger = make_shared<PSLogger>(PSLogger());
	}
}

Parser::~Parser()
{
}

std::optional<ParsedGame> Parser::parse_file()
{
	m_parsed_game = ParsedGame();

	if(m_line_counter != 1)
	{
		m_logger->log(PSLogger::LogType::Error, m_parser_log_cat, "already tried parsing a file");
		return nullopt;
	}
	//m_line_counter = 1;

	if(m_text_provider->is_valid())
	{
		m_logger->log(PSLogger::LogType::Log, m_parser_log_cat, "Parsing file");

		m_current_file_section = FileSection::Prelude;

		while(m_text_provider->is_valid() && !m_has_error)
		{
			switch (m_current_file_section)
			{
			case FileSection::None:
				detect_error("error cannot parse None section");
				break;
			case FileSection::Prelude:
				parse_prelude();
				break;
			case FileSection::Objects:
				parse_objects();
				break;
			case FileSection::Legend:
				parse_legend();
				break;
			case FileSection::Sounds:
				parse_sounds();
				break;
			case FileSection::CollisionLayers:
				parse_collision_layers();
				break;

			case FileSection::Rules:
				parse_rules();
				break;

			case FileSection::WinConditions:
				parse_win_conditions();
				break;

			case FileSection::Levels:
				parse_levels();
				break;

			default:
				detect_error("parser not fully implemented yet");
				break;
			}
		}

		return  std::optional<ParsedGame>(m_parsed_game);
	}
	else
	{
		m_logger->log(PSLogger::LogType::Error, m_parser_log_cat, "Invalid text provider. Parsing will fail.");
		return nullopt;
	}
	
}

void Parser::try_change_file_section(FileSection p_new_file_section)
{
	m_logger->log(PSLogger::LogType::Log, m_parser_log_cat, "Changing Section -------------------------" + enum_to_str(p_new_file_section,to_file_section).value_or("error converting string"));
	m_current_file_section = p_new_file_section;
	//check that sections are in the correct order;
}

void Parser::parse_prelude()
{
	m_logger->log(PSLogger::LogType::Log, m_parser_log_cat, "Starting prelude parsing");

	bool line_beggining = true;

	while(m_text_provider->is_valid() && m_current_file_section == FileSection::Prelude)
	{
			switch(m_text_provider->advance())
			{
				case '=':
				//ignore '=' character
					line_beggining = false;
					break;
				case '(':
					parse_comment();
					line_beggining = false;
					break;
				case '\n':
					//cout << "--\n";
					m_line_counter ++;
					line_beggining = true;
					break;
				default:
					if(line_beggining)
					{
						//should also detect if we're changing section
						parse_prelude_identifier();
					}
					else
					{
						parse_prelude_literal();
					}
					line_beggining = false;
				break;
			}
			
	}

	m_logger->log(PSLogger::LogType::Log, m_parser_log_cat, "Finished prelude parsing");
}

void Parser::parse_prelude_identifier()
{
	string identifier_string = parse_word();

	//cout << "identifier :" << identifier_string << "\n";

	if(str_to_enum(identifier_string, to_file_section).value_or(FileSection::None) != FileSection::None)
	{
		FileSection fs = str_to_enum(identifier_string, to_file_section).value_or(FileSection::None);
		try_change_file_section(fs);
	}
	else if(str_to_enum(identifier_string, ParsedGame::to_prelude_token_type).value_or(ParsedGame::PreludeTokenType::None) != ParsedGame::PreludeTokenType::None)
	{
		m_parsed_game.prelude_tokens.push_back(Token<ParsedGame::PreludeTokenType>(str_to_enum(identifier_string, ParsedGame::to_prelude_token_type).value(),"", m_line_counter));
	}
	else
	{
		detect_error("unidentified identifier : \"" + identifier_string + "\" in the prelude");
	}
}

void Parser::parse_prelude_literal()
{
	string current_literal_string = "";
	if(m_text_provider->get_current_char() != ' ')
	{
		current_literal_string.push_back(m_text_provider->get_current_char());
	}

	bool detected_literal_end = false;
	while(m_text_provider->is_valid() && !detected_literal_end)
	{
		switch(m_text_provider->advance())
		{
			case '(':
				parse_comment();
				break;
			case '\n':
				detected_literal_end = true;
				m_text_provider->reverse();
			break;

			default:
				current_literal_string.push_back(m_text_provider->get_current_char());
			break;
		}
	}

	//cout << "literal :" << current_literal_string << "\n";

	m_parsed_game.prelude_tokens.push_back(Token<ParsedGame::PreludeTokenType>(ParsedGame::PreludeTokenType::Literal,current_literal_string, m_line_counter));
}

void Parser::parse_objects()
{
	m_logger->log(PSLogger::LogType::Log, m_parser_log_cat, "Starting objetcs parsing");

	while(m_text_provider->is_valid() && m_current_file_section == FileSection::Objects)
	{
			switch(m_text_provider->advance())
			{
				case ' ':
				case '=':
				//ignore '=' character
					break;
				case '(':
					parse_comment();
					break;
				case '\n':
					//cout << "--\n";
					m_line_counter ++;
					break;
				case '#':
					parse_objects_color_hex_code();
					break;
				default:
					if(is_pixel(m_text_provider->get_current_char()))
					{
						string pixel_str = "";
						pixel_str.push_back(m_text_provider->get_current_char());
						Token<ParsedGame::ObjectsTokenType> pixel(ParsedGame::ObjectsTokenType::Pixel, pixel_str, m_line_counter);
						m_parsed_game.objects_tokens.push_back(pixel);
					}
					else
					{
						string parsed_word = parse_word();

						if(str_to_enum(parsed_word, to_file_section).value_or(FileSection::None) != FileSection::None)
						{
							FileSection fs = str_to_enum(parsed_word, to_file_section).value_or(FileSection::None);
							try_change_file_section(fs);
						}
						else if(str_to_enum(parsed_word, ParsedGame::to_objects_color).value_or(ParsedGame::ObjectsColor::None) != ParsedGame::ObjectsColor::None)
						{
							m_parsed_game.objects_tokens.push_back(Token<ParsedGame::ObjectsTokenType>(ParsedGame::ObjectsTokenType::ColorName, parsed_word, m_line_counter));
						}
						else
						{
							m_parsed_game.objects_tokens.push_back(Token<ParsedGame::ObjectsTokenType>(ParsedGame::ObjectsTokenType::Literal, parsed_word, m_line_counter));
						}
					}	
				break;
			}
			
	}

	m_logger->log(PSLogger::LogType::Log, m_parser_log_cat, "Finished objects parsing");
}

void Parser::parse_objects_color_hex_code()
{
	//todo make a more robust parse function and detect incorrect hex codes
	string color_hex_code_str = parse_word();
	Token<ParsedGame::ObjectsTokenType> color_hex_code(ParsedGame::ObjectsTokenType::ColorHexCode, color_hex_code_str, m_line_counter);
	m_parsed_game.objects_tokens.push_back(color_hex_code);
}

bool Parser::is_pixel(char p_char)
{
	return (p_char >= '0' && p_char <= '9') || p_char == '.';
}

void Parser::parse_legend()
{
	m_logger->log(PSLogger::LogType::Log, m_parser_log_cat, "Starting legend parsing");

	while(m_text_provider->is_valid() && m_current_file_section == FileSection::Legend)
	{
			switch(m_text_provider->advance())
			{
				case ' ':
					break;
				case '(':
					parse_comment();
					break;
				case '\n':
					m_parsed_game.legend_tokens.push_back(Token<ParsedGame::LegendTokenType>(ParsedGame::LegendTokenType::Return,"",m_line_counter));
					m_line_counter ++;
					break;
				case '=':
					if(m_text_provider->peek() == '=')
					{
						parse_equals_row();
					}
					else
					{
						m_parsed_game.legend_tokens.push_back(Token<ParsedGame::LegendTokenType>(ParsedGame::LegendTokenType::Equal,"",m_line_counter));
					}	
					break;
				default:
					string parsed_word = parse_word();

					FileSection fs = str_to_enum(parsed_word, to_file_section).value_or(FileSection::None);
					ParsedGame::LegendTokenType ltt = str_to_enum(parsed_word, ParsedGame::to_legend_token_type).value_or(ParsedGame::LegendTokenType::None);

					if(fs != FileSection::None)
					{
						try_change_file_section(fs);
					}
					else if(ltt != ParsedGame::LegendTokenType::None && ltt != ParsedGame::LegendTokenType::Identifier)
					{
						m_parsed_game.legend_tokens.push_back(Token<ParsedGame::LegendTokenType>(ltt, "", m_line_counter));
					}
					else
					{
						m_parsed_game.legend_tokens.push_back(Token<ParsedGame::LegendTokenType>(ParsedGame::LegendTokenType::Identifier, parsed_word, m_line_counter));
					}
				break;
			}
			
	}

	m_logger->log(PSLogger::LogType::Log, m_parser_log_cat, "Finished legend parsing");
}

void Parser::parse_sounds()
{
	//todo proper implementation of this section
	m_logger->log(PSLogger::LogType::Log, m_parser_log_cat, "Starting sounds parsing : Ignoring this section for now");

	while(m_text_provider->is_valid() && m_current_file_section == FileSection::Sounds)
	{
			switch(m_text_provider->advance())
			{
				case ' ':
					break;
				case '(':
					parse_comment();
					break;
				case '\n':
					m_line_counter ++;
					break;
				default:
					string parsed_word = parse_word();

					FileSection fs = str_to_enum(parsed_word, to_file_section).value_or(FileSection::None);

					if(fs != FileSection::None)
					{
						try_change_file_section(fs);
					}
				break;
			}
			
	}

	m_logger->log(PSLogger::LogType::Log, m_parser_log_cat, "Finished sounds parsing");
}

void Parser::parse_collision_layers()
{
	m_logger->log(PSLogger::LogType::Log, m_parser_log_cat, "Starting collision layers parsing");

	while(m_text_provider->is_valid() && m_current_file_section == FileSection::CollisionLayers)
	{
			switch(m_text_provider->advance())
			{
				case '=':
					parse_equals_row();
					break;
				case ' ':
					break;
				case '(':
					parse_comment();
					break;
				case ',':
					m_parsed_game.collision_layers_tokens.push_back(Token<ParsedGame::CollisionLayersTokenType>(ParsedGame::CollisionLayersTokenType::Comma,"",m_line_counter));
					break;
				case '\n':
					m_parsed_game.collision_layers_tokens.push_back(Token<ParsedGame::CollisionLayersTokenType>(ParsedGame::CollisionLayersTokenType::Return,"",m_line_counter));
					m_line_counter ++;
					break;
				default:
					string parsed_word = parse_word();

					FileSection fs = str_to_enum(parsed_word, to_file_section).value_or(FileSection::None);

					if(fs != FileSection::None)
					{
						try_change_file_section(fs);
					}
					else
					{
						m_parsed_game.collision_layers_tokens.push_back(Token<ParsedGame::CollisionLayersTokenType>(ParsedGame::CollisionLayersTokenType::Identifier,parsed_word,m_line_counter));
					}
					
				break;
			}
			
	}

	m_logger->log(PSLogger::LogType::Log, m_parser_log_cat, "Finished collision layers parsing");
}

void Parser::parse_rules()
{
	m_logger->log(PSLogger::LogType::Log, m_parser_log_cat, "Starting rules parsing");

	while(m_text_provider->is_valid() && m_current_file_section == FileSection::Rules)
	{
			switch(m_text_provider->advance())
			{
				case '=':
					parse_equals_row();
					break;
				case ' ':
					break;
				case '(':
					parse_comment();
					break;
				case ',':
					detect_error("Unexpected comma.");
					break;
				case '\n':
					m_parsed_game.rules_tokens.push_back(Token<ParsedGame::RulesTokenType>(ParsedGame::RulesTokenType::Return,"",m_line_counter));
					m_line_counter ++;
					break;
				case '[':
					m_parsed_game.rules_tokens.push_back(Token<ParsedGame::RulesTokenType>(ParsedGame::RulesTokenType::LeftBracket,"",m_line_counter));
					break;
				case ']':
					m_parsed_game.rules_tokens.push_back(Token<ParsedGame::RulesTokenType>(ParsedGame::RulesTokenType::RightBracket,"",m_line_counter));
					break;
				case '|':
					m_parsed_game.rules_tokens.push_back(Token<ParsedGame::RulesTokenType>(ParsedGame::RulesTokenType::Bar,"",m_line_counter));
					break;
				case '-':
					if(m_text_provider->peek() == '>')
					{
						m_text_provider->advance();
						m_parsed_game.rules_tokens.push_back(Token<ParsedGame::RulesTokenType>(ParsedGame::RulesTokenType::Arrow,"",m_line_counter));
					}
					else
					{
						detect_error("unexpected -.");
					}
					break;
				case 'v':
				case '>':
				case '^':
				case '<':
					if(m_text_provider->peek() == ' ')
					{					
						ParsedGame::RulesTokenType token_type = ParsedGame::RulesTokenType::RelativeRight;
						if(m_text_provider->get_current_char() == '<')
						{
							token_type = ParsedGame::RulesTokenType::RelativeLeft;
						}
						else if(m_text_provider->get_current_char() == '^')
						{
							token_type = ParsedGame::RulesTokenType::RelativeUp;
						}
						else if(m_text_provider->get_current_char() == 'v')
						{
							token_type = ParsedGame::RulesTokenType::RelativeDown;
						}
						else
						{ 
							//token_type is already set for > char
						}
						m_parsed_game.rules_tokens.push_back(Token<ParsedGame::RulesTokenType>(token_type,"",m_line_counter));
					}
					else
					{
						parse_rules_word();
					}
					break;
				case '.':
					if(m_text_provider->peek() == '.' && m_text_provider->peek_next() == '.')
					{
						m_text_provider->advance();
						m_text_provider->advance();
						m_parsed_game.rules_tokens.push_back(Token<ParsedGame::RulesTokenType>(ParsedGame::RulesTokenType::Dots,"",m_line_counter));
					}
					break;
				default:
					parse_rules_word();
				break;
			}
			
	}

	m_logger->log(PSLogger::LogType::Log, m_parser_log_cat, "Finished rules parsing");
}

void Parser::parse_rules_word()
{
	string parsed_word = parse_word();

	FileSection fs = str_to_enum(parsed_word, to_file_section).value_or(FileSection::None);

	if(fs != FileSection::None)
	{
		try_change_file_section(fs);
	}
	else
	{
		ci_equal comp_equal;

		vector<string> reserved_words = {
			"NO",
			"LATE",
			"AGAIN",
			"MOVING",
			"STATIONARY",
        	"PARALLEL",
        	"PERPENDICULAR",
        	"HORIZONTAL",
        	"VERTICAL",
			"ORTHOGONAL",
			"ACTION",
			"UP",
			"DOWN",
			"LEFT",
			"RIGHT",
			"CHECKPOINT",
			"CANCEL",
			"WIN",
			"MESSAGE",
			"SFX0", "SFX1","SFX2","SFX3","SFX4","SFX5","SFX6","SFX7","SFX8","SFX9","SFX10"};

		auto found_reserved_word = std::find_if(reserved_words.begin(),reserved_words.end(),[&](const string& s){
			return comp_equal(s, parsed_word);
		});

		if(found_reserved_word != reserved_words.end())
		{
			ParsedGame::RulesTokenType token_type = str_to_enum(parsed_word,ParsedGame::to_rules_token_type).value_or(ParsedGame::RulesTokenType::None);
			if(token_type == ParsedGame::RulesTokenType::None)
			{
				//todo more explicit error message
				detect_error(parsed_word);
			}

			m_parsed_game.rules_tokens.push_back(Token<ParsedGame::RulesTokenType>(token_type,"",m_line_counter));
		}
		else
		{
			m_parsed_game.rules_tokens.push_back(Token<ParsedGame::RulesTokenType>(ParsedGame::RulesTokenType::Identifier,parsed_word,m_line_counter));
		}
	}
}

void Parser::parse_win_conditions()
{
	m_logger->log(PSLogger::LogType::Log, m_parser_log_cat, "Starting win conditions parsing");

	while(m_text_provider->is_valid() && m_current_file_section == FileSection::WinConditions)
	{
			switch(m_text_provider->advance())
			{
				case '=':
					parse_equals_row();
					break;
				case ' ':
					break;
				case '(':
					parse_comment();
					break;
				case '[':
				case ']':
				case '|':
				case ',':
					detect_error("Unexpected symbol.");
					break;
				case '\n':
					m_parsed_game.win_conditions_tokens.push_back(Token<ParsedGame::WinConditionsTokenType>(ParsedGame::WinConditionsTokenType::Return,"",m_line_counter));
					m_line_counter ++;
					break;
				default:
					string parsed_word = parse_word();

					FileSection fs = str_to_enum(parsed_word, to_file_section).value_or(FileSection::None);

					if(fs != FileSection::None)
					{
						try_change_file_section(fs);
					}
					else
					{
						ci_equal comp_equal;

						vector<string> reserved_words = {"NO", "ON", "ALL", "SOME"};

						auto found_reserved_word = std::find_if(reserved_words.begin(),reserved_words.end(),[&](const string& s){
							return comp_equal(s, parsed_word);
						});

						if(found_reserved_word != reserved_words.end())
						{
							ParsedGame::WinConditionsTokenType token_type = str_to_enum(parsed_word,ParsedGame::to_win_conditions_token_type).value_or(ParsedGame::WinConditionsTokenType::None);
							if(token_type == ParsedGame::WinConditionsTokenType::None)
							{
								//todo more explicit error message
								detect_error(parsed_word);
							}

							m_parsed_game.win_conditions_tokens.push_back(Token<ParsedGame::WinConditionsTokenType>(token_type,"",m_line_counter));
						}
						else
						{
							m_parsed_game.win_conditions_tokens.push_back(Token<ParsedGame::WinConditionsTokenType>(ParsedGame::WinConditionsTokenType::Identifier,parsed_word,m_line_counter));
						}
					}
				break;
			}
			
	}

	m_logger->log(PSLogger::LogType::Log, m_parser_log_cat, "Finished win conditions parsing");
}

void Parser::parse_levels()
{
	m_logger->log(PSLogger::LogType::Log, m_parser_log_cat, "Starting levels parsing");
	bool is_line_beggining = false; 
	while(m_text_provider->is_valid() && m_current_file_section == FileSection::Levels)
	{
			//cout << m_current_char << " ";
			switch(m_text_provider->advance())
			{
				case '=':
					parse_equals_row();
					is_line_beggining = false;
					break;
				case ' ':
				case '\0': //todo this is necessary because of the way the string textprovider works but this should probably be removed and the textprovider fixed
					break;
				case '(':
					parse_comment();
					break;
				case '\n':
					is_line_beggining = true;
					m_parsed_game.levels_tokens.push_back(Token<ParsedGame::LevelsTokenType>(ParsedGame::LevelsTokenType::Return,"",m_line_counter));
					m_line_counter ++;
					break;
				default:
					if((m_text_provider->get_current_char() != 'm' && m_text_provider->get_current_char() != 'M') 
					|| (!is_line_beggining || !try_parse_levels_message()))
					{
						string tile = "";
						tile.push_back(m_text_provider->get_current_char());
						m_parsed_game.levels_tokens.push_back(Token<ParsedGame::LevelsTokenType>(ParsedGame::LevelsTokenType::LevelTile,tile,m_line_counter));
					}
					is_line_beggining = false;
				break;
			}
			
	}

	m_logger->log(PSLogger::LogType::Log, m_parser_log_cat, "Finished levels parsing");
}

bool Parser::try_parse_levels_message()
{
	string temp_buffer = "";
	temp_buffer.push_back(m_text_provider->get_current_char());

	bool detected_word_end = false;
	while(m_text_provider->is_valid() && !detected_word_end)
	{
		switch(m_text_provider->advance())
		{
			case '\n':
				detected_word_end = true;
				m_text_provider->reverse();
			break;
				case ' ':
				detected_word_end = true;
			break;
			default:
				temp_buffer.push_back(m_text_provider->get_current_char());
			break;
		}
	}

	ci_equal equal_comp;
	if(equal_comp("Message",temp_buffer))
	{
		//add token message and parse the rest
		m_parsed_game.levels_tokens.push_back(Token<ParsedGame::LevelsTokenType>(ParsedGame::LevelsTokenType::MESSAGE,"",m_line_counter));

		temp_buffer.clear();

		detected_word_end = false;
		while(m_text_provider->is_valid() && !detected_word_end)
		{
			switch(m_text_provider->advance())
			{
				case '\n':
					detected_word_end = true;
					m_text_provider->reverse();
				break;
				default:
					temp_buffer.push_back(m_text_provider->get_current_char());
				break;
			}
		}
		m_parsed_game.levels_tokens.push_back(Token<ParsedGame::LevelsTokenType>(ParsedGame::LevelsTokenType::MessageContent,temp_buffer,m_line_counter));

		return true;
	}
	else
	{
		//reverse everything and return false, we couldn't properly parse a message token
		for(auto it = temp_buffer.rbegin(); it != temp_buffer.rend(); it++)
		{
			m_text_provider->reverse();
		}
		m_text_provider->advance(); //so the 'm' or 'M' gets back to being the m_current_char
		return false;
	}
}

void Parser::parse_equals_row()
{
	while(m_text_provider->is_valid() && m_text_provider->peek() == '=')
	{
		m_text_provider->advance();
	}
}

//todo : since some tokens are excluded here but not handled in every section parser, this may lead to infinite loop if the file is not correct
string Parser::parse_word()
{
	string current_string = "";
	current_string.push_back(m_text_provider->get_current_char());

	bool detected_word_end = false;
	while(m_text_provider->is_valid() && !detected_word_end)
	{
		switch(m_text_provider->advance())
		{
			case '=':
			case ',':
			case '(':
			case '[':
			case ']':
			case '|':
			case '\n':
			case ' ':
				detected_word_end = true;
				m_text_provider->reverse();
			break;

			default:
				current_string.push_back(m_text_provider->get_current_char());
			break;
		}
	}

	return current_string;
}

void Parser::parse_comment(int p_comment_level /*= 0*/)
{
	//cout << "starting comment parsing level : " << p_comment_level << "\n";

	bool continue_parsing = true;

	while(m_text_provider->is_valid() && continue_parsing)
	{
		switch (m_text_provider->advance())
		{
		case '\n':
			++m_line_counter;
			break;
		case '(':
			parse_comment(p_comment_level + 1);
			break;
		case ')':
			continue_parsing = false;
			break;
		
		default:
			break;
		}
	}

	if(!m_text_provider->is_valid())
	{
		detect_error("Unexpected end of file : ");
	}

	//cout << "Finishing comment parsing level : " << p_comment_level << "\n";
}

void Parser::detect_error(string p_error_msg)
{
	m_has_error = true;
	m_logger->log(PSLogger::LogType::Error, m_parser_log_cat, "(l." + to_string(m_line_counter) + ") : " + p_error_msg);
}
