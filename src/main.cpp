
#include <iostream>
#include <string>
#include <filesystem>

#include "Parser.hpp"
#include "Compiler.hpp"
#include "PSEngine.hpp"
#include "EnumHelpers.hpp"

const int TEST_RECORD_VERSION = 1;

optional<CompiledGame> compile_game(string file_path)
{
    shared_ptr<PSLogger> logger = make_shared<PSLogger>(PSLogger());
    logger->only_log_errors = true;

    std::ifstream ifs(file_path);
    std::string content( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );

    ParsedGame parsed_game = Parser::parse_from_string(content,logger).value_or(ParsedGame());
    //parsed_game.print_parsed_game(true,true,true,true,true,true,true);

    Compiler puzzle_compiler(logger);
    return puzzle_compiler.compile_game(parsed_game);
}

void parse_and_send_game_input(PSEngine& p_engine, char input)
{
    if(input == 'r')
    {
        p_engine.restart_level();
        p_engine.print_game_state();
        return;
    }
    else if(input == 'a')
    {
        p_engine.undo();
        p_engine.print_game_state();
        return;
    }
    else if(input == 'l')
    {
        p_engine.print_operation_history();
        return;
    }

    PSEngine::InputType input_type = PSEngine::InputType::None;

    switch (input)
    {
    case 'z':
        input_type = PSEngine::InputType::Up;
        break;
    case 'q':
        input_type = PSEngine::InputType::Left;
        break;
    case 's':
        input_type = PSEngine::InputType::Down;
        break;
    case 'd':
        input_type = PSEngine::InputType::Right;
        break;
    case 'e':
        input_type = PSEngine::InputType::Action;
        break;
    default:
        break;
    }

    if(input_type != PSEngine::InputType::None)
    {
        p_engine.receive_input(input_type);
        p_engine.print_game_state();
    }
}

void load_and_run_game(string file_path)
{
    std::optional<CompiledGame> compiled_game_opt = compile_game(file_path);
    if(!compiled_game_opt.has_value())
    {
        return;
    }
    CompiledGame compiled_game = compiled_game_opt.value();
    compiled_game.print();

    PSEngine engine;
    engine.load_game(compiled_game);
    engine.Load_first_level();

    engine.print_game_state();

    while(true)
    {

        if(engine.is_level_won())
        {
            cout << "Level Complete !\n Press r to restart the level or n to load the next one.\n";

            char input = (char)getchar();

            if(input == 'r')
            {
                engine.restart_level();
                engine.print_game_state();
            }
            else if(input == 'n')
            {
                engine.load_next_level();
                engine.print_game_state();
            }

        }
        else
        {
            char input = (char)getchar();

            parse_and_send_game_input(engine,input);
        }
    }
}

void run_game_and_record_test_file(string resources_folder_path, string file_name)
{
    string file_path = resources_folder_path + file_name;

    std::optional<CompiledGame> compiled_game_opt = compile_game(file_path);
    if(!compiled_game_opt.has_value())
    {
        cout << "error: cannot record a test file for a file that does not compile.\n";
        return;
    }
    CompiledGame compiled_game = compiled_game_opt.value();
    compiled_game.print();

    PSEngine engine;
    engine.load_game(compiled_game);

    ci_equal equal_op;

    cout << "starting test file recording for for game " << file_path << "\n";

    ofstream record_file;
    record_file.open (resources_folder_path +file_name + ".test_record", ios::out | ios::trunc);
    if(!record_file.is_open())
    {
        cout << "error: could not create a record file in the directory\n";
        return;
    }

    record_file << "format version " << TEST_RECORD_VERSION << "\n";
    record_file << file_name << "\n";

    while(true)
    {
        cout << "detected  " << compiled_game.levels.size() << " levels.\n input the index of a level to record a test run\n input quit to quit the record session\n";

        string input1;
        getline(cin,input1);

        if(equal_op(input1,"quit"))
        {
            break;
        }
        else
        {
            int level_idx = -1;
            try{
                level_idx = stoi(input1);
            }
            catch(...)
            {
                cout << "error : incorrect input\n";
                continue;
            }

            if(level_idx >= compiled_game.levels.size())
            {
                cout << "error : level index out of range\n";
                continue;
            }

            record_file << level_idx << "\n";

            engine.load_level(level_idx);
            engine.print_game_state();

            while(true)
            {
                string input2;
                getline(cin,input2);
                if(equal_op(input2,"quit"))
                {
                    record_file << "notwon\n";
                    break;
                }
                else
                {
                    if(input2.size() == 1)
                    {
                        parse_and_send_game_input(engine, input2[0]);
                        record_file << input2 << "\n";
                        engine.print_game_state();

                        if(engine.is_level_won())
                        {
                            record_file << "won\n";
                            break;
                        }
                    }
                    else
                    {
                        cout << "error : please only enter one character at a time";
                    }
                }

            }

        }
    }

    record_file.close();
}

bool run_tests(string directory_path)
{
    shared_ptr<PSLogger> logger = make_shared<PSLogger>(PSLogger());
    logger->only_log_errors = true;

    ci_equal equal_op;

    bool has_error = false;

    cout << "Running the tests\n";

    for (const auto & entry : std::filesystem::directory_iterator(directory_path))
    {
        if(equal_op(entry.path().extension().string(),".test_record"))
        {
            cout << "found " << entry.path() <<"\n";

            ifstream test_record_f;
            test_record_f.open(entry.path().string());

            if(!test_record_f.is_open())
            {
                cout << "error: could not open " << entry.path() << "\n";
                has_error = true;
                continue;
            }

            string record_line = "";
            getline(test_record_f,record_line);

            if(!equal_op("format version "+to_string(TEST_RECORD_VERSION), record_line))
            {
                cout << "error: incompatible format version, test wont be possible on this game.\n";
                has_error = true;
                continue;
            }

            getline(test_record_f,record_line);

            std::optional<CompiledGame> compiled_game_opt = compile_game(directory_path + record_line);
            if(!compiled_game_opt.has_value())
            {
                cout << "error: cannot compile associated game, test wont be possible on this game.\n";
                has_error = true;
                continue;
            }
            CompiledGame compiled_game = compiled_game_opt.value();

            PSEngine engine(logger);
            engine.load_game(compiled_game);

            bool need_level_idx = true;

            while(getline(test_record_f,record_line))
            {
                if(need_level_idx)
                {
                    try
                    {
                        int level_idx = stoi(record_line);
                        engine.load_level(level_idx);
                        need_level_idx = false;
                    }
                    catch(...)
                    {
                        cout << "error : was expecting a level index\n";
                        has_error= true;
                        break;
                    }

                }
                else
                {
                    if(equal_op(record_line, "won") || equal_op(record_line,"notwon"))
                    {
                        if((equal_op(record_line, "won") && engine.is_level_won() )
                        || (equal_op(record_line, "notwon") && !engine.is_level_won() ))
                        {
                            //empty
                            //todo reverse the test
                        }
                        else
                        {
                            cout << "error : level result does not match record file.\n";
                            has_error = true;
                        }

                        need_level_idx = true;
                    }
                    else
                    {
                        if(record_line.size() != 1)
                        {
                            cout << "error : was expecting a single character\n";
                            has_error = true;
                            break;
                        }

                        parse_and_send_game_input(engine,record_line[0]);

                        if(engine.is_level_won())
                        {
                            getline(test_record_f,record_line);

                            if(!equal_op(record_line, "won"))
                            {
                                cout << "error : was not expecting the level to be won at this point\n";
                                has_error = true;
                                break;
                            }

                            need_level_idx = true;
                        }
                    }
                }
            }

            test_record_f.close();
        }
    }

    return !has_error;
}

int main(int argc, char *argv[])
{
    string resources_folder_path = "resources/";
    string file_name = "sokobang.txt";

    bool has_error =false;

    bool test_requested = false;
    bool record_requested = false;

    ci_equal equal_op;

    if(argc >= 2)
    {
        if(equal_op(argv[1],"tests"))
        {
            test_requested = true;
        }
        else
        {
            file_name = argv[1];
        }
    }

    if(argc >= 3)
    {
        if(test_requested)
        {
            cout << "error : no other argument can be passed if a test is requested\n";
            has_error = true;
        }
        else if(equal_op(argv[2],"record"))
        {
            record_requested = true;
        }
        else
        {
            cout << "error : incorrect argument\n";
            has_error = true;
        }

    }

    if( argc > 3)
    {
    	cout << "error : too many command arguments\n";
        has_error = true;
    }

    if(has_error)
    {
        return EXIT_FAILURE;
    }

    if(test_requested)
    {
        if(run_tests(resources_folder_path))
        {
            cout << "All tests completed with success !\n";
        }
        else
        {
            cout << "Errors detected while testing, there may have been a regression with the engine\n";
        }

    }
    else
    {
        string file_path = resources_folder_path + file_name;

        if(record_requested)
        {
            run_game_and_record_test_file(resources_folder_path,file_name);
        }
        else
        {
            load_and_run_game(file_path);
        }

    }

	return 0;
}
