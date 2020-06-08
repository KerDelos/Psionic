#pragma once

#include <memory>
#include <optional>

#include "CompiledGame.hpp"
#include "ParsedGame.hpp"
#include "PSLogger.hpp"

class Compiler{

public:

    Compiler(shared_ptr<PSLogger> p_logger);
    std::optional<CompiledGame> compile_game(const ParsedGame& p_parsed_game);

protected:
    shared_ptr<PSLogger> m_logger;

    CompiledGame m_compiled_game;
    bool m_has_error = false;

    static const string m_compiler_log_cat;

    void compile_prelude(const vector<Token<ParsedGame::PreludeTokenType>>& p_prelude_tokens);

    void compile_objects(const vector<Token<ParsedGame::ObjectsTokenType>>& p_objects_tokens);

    void compile_legend(const vector<Token<ParsedGame::LegendTokenType>>& p_legend_tokens);

    void compile_collision_layers(const vector<Token<ParsedGame::CollisionLayersTokenType>>& p_collision_layer_tokens);

    void compile_rules(const vector<Token<ParsedGame::RulesTokenType>>& p_rules_tokens);

    void compile_win_conditions(const vector<Token<ParsedGame::WinConditionsTokenType>>& p_win_conditions_tokens);

    void compile_levels(const vector<Token<ParsedGame::LevelsTokenType>>& p_levels_tokens);

    void reference_collision_layers_in_objects();

    void verify_rules();

    bool check_identifier_validity(const string& p_id, int p_identifier_line_numbler, bool p_should_already_exist);
    weak_ptr<CompiledGame::Object> get_obj_by_id(const string& p_id);

    bool retrieve_player_object();
    bool retrieve_background_objects();

    weak_ptr<CompiledGame::Object> m_background_object;
    weak_ptr<CompiledGame::PrimaryObject> m_default_background_object;

    template<typename T>
    void detect_error(Token<T> p_token, string p_error_msg, bool p_is_warning = false);
    void detect_error(int p_line_number, string p_error_msg, bool p_is_warning = false);

    
};
