#pragma once

#include <string>
#include <map>

#include "EnumHelpers.hpp"
#include "CompiledGame.hpp"
#include "PSLogger.hpp"


#define PS_LOG(p_log_msg){\
    if(m_config.log_verbosity <= PSLogger::LogType::Log)\
    {\
        m_logger->log(PSLogger::LogType::Log, m_engine_log_cat, p_log_msg);\
    }\
}

#define PS_LOG_VERBOSE(p_log_msg){\
    if(m_config.log_verbosity <= PSLogger::LogType::VerboseLog)\
    {\
        m_logger->log(PSLogger::LogType::VerboseLog, m_engine_log_cat, p_log_msg);\
    }\
}

#define PS_LOG_WARNING(p_log_msg){\
    if(m_config.log_verbosity <= PSLogger::LogType::Warning)\
    {\
        m_logger->log(PSLogger::LogType::Warning, m_engine_log_cat, p_log_msg);\
        if(m_config.log_operation_history_after_error)\
        {\
            print_operation_history();\
        }\
    }\
}

#define PS_LOG_ERROR(p_log_msg){\
    if(m_config.log_verbosity <= PSLogger::LogType::Error)\
    {\
        m_logger->log(PSLogger::LogType::Error, m_engine_log_cat, p_log_msg);\
        if(m_config.log_operation_history_after_error)\
        {\
            print_operation_history();\
        }\
    }\
}

class PSEngine
{
public:
    struct Config
    {
        PSLogger::LogType log_verbosity = PSLogger::LogType::Warning;
        bool log_operation_history_after_error = true;
        bool add_ticks_to_operation_history = false;
    };

    enum ObjectMoveType
    {
        None,
        Up,
        Down,
        Left,
        Right,
        Action,
        Stationary,
    };

    struct Cell
    {
        int x = -1;
        int y = -1;
        map<shared_ptr<CompiledGame::PrimaryObject>,ObjectMoveType> objects;

        optional<shared_ptr<CompiledGame::PrimaryObject>> find_colliding_object(shared_ptr<CompiledGame::PrimaryObject> obj) const;
    };

    struct Level
    {
        int level_idx = -1;
        int width = -1;
        int height = -1;
        vector<Cell> cells;
    };

    enum class AbsoluteDirection{
        None,
        Up,
        Down,
        Left,
        Right,
    };

    struct PatternMatchInformation
    {
        vector<int> wildcard_pattern_cell_indexes;
        vector<int> wildcard_match_distances;
        int x =-1;
        int y =-1;
    };

    struct ObjectDelta
    {
        int cell_x = -1;
        int cell_y = -1;
        shared_ptr<CompiledGame::PrimaryObject> object;
        CompiledGame::ObjectDeltaType type;

        ObjectDelta(int p_cell_x, int p_cell_y, shared_ptr<CompiledGame::PrimaryObject> p_object, CompiledGame::ObjectDeltaType p_type)
        :cell_x(p_cell_x), cell_y(p_cell_y), object(p_object), type(p_type){};

        friend bool operator==(const ObjectDelta& lhs, const ObjectDelta& rhs){
            bool result = lhs.object == rhs.object;
            result &= lhs.type == rhs.type;
            result &= lhs.cell_x == rhs.cell_x;
            result &= lhs.cell_y == rhs.cell_y;
            return result;
        }
    };

    struct RuleApplicationDelta
    {
        AbsoluteDirection rule_direction;
        vector<PatternMatchInformation> match_infos;
        vector<ObjectDelta> object_deltas;

        friend bool operator==(const RuleApplicationDelta& lhs, const RuleApplicationDelta& rhs){
            return lhs.object_deltas == rhs.object_deltas;
        }
    };

    struct MovementDelta
    {
        int origin_x = -1;
        int origin_y = -1;
        int destination_x = -1;
        int destination_y = -1;
        AbsoluteDirection move_direction;
        shared_ptr<CompiledGame::PrimaryObject> object;
        //bool moved_successfully = false; //todo also register failed movements
    };

    struct RuleDelta //todo this class should probably be renamed since it can now hold movement deltas
    {
        bool is_movement_resolution = false;

        //params for a movement resolution delta
        vector<MovementDelta> movement_deltas;

        //params for a rule delta
        CompiledGame::Rule rule_applied;
        vector<RuleApplicationDelta> rule_application_deltas;
    };

    enum class InputType
    {
        None,
        Up,
        Down,
        Left,
        Right,
        Action,
    };



    enum class OperationType{
        None,
        Input,
        LoadLevel,
        Tick,
        Undo,
        Restart,
        LoadGame,
     };

    struct SubturnHistory
    {
        vector<RuleDelta> steps;

        vector<CompiledGame::CommandType> gather_all_subturn_commands() const;
    };

    struct Operation{
        OperationType operation_type = OperationType::None;

        InputType input_type = InputType::None;
        int loaded_level = -1;
        float delta_time = -1;
        string loaded_game_title = ""; //leave empty if not specified in the metedata ?

        Operation(OperationType p_op_type = OperationType::None, InputType p_input_type = InputType::None, int p_loaded_level = -1, string p_loaded_game_title = "")
        :operation_type(p_op_type),input_type(p_input_type),loaded_level(p_loaded_level),loaded_game_title(p_loaded_game_title)
        {};
    };

    static std::map<string,OperationType, ci_less> to_operation_type;
    static std::map<string,AbsoluteDirection, ci_less> to_absolute_direction;
    static std::map<string,InputType, ci_less> to_input_type;
    static std::map<string,ObjectMoveType, ci_less> to_object_move_type;

    PSEngine(shared_ptr<PSLogger> p_logger = nullptr);

    void load_game(const CompiledGame& p_game_to_load);

    void Load_first_level();

    void load_level(int p_level_idx);

    optional<vector<SubturnHistory>> receive_input(InputType p_input);

    optional<vector<SubturnHistory>> tick(float p_delta_time);

    void restart_level();
    bool undo();

    void load_next_level();

    bool is_level_won() const;

    int get_number_of_levels() const {return (int)m_compiled_game.levels.size();}

    void print_game_state(); //todo making this const will require a few changes and maybe some mutables

    Level get_level_state() const {return m_current_level;};

    vector<SubturnHistory> get_turn_deltas() {return m_turn_history;}

    string operation_history_to_string() const;
    void print_operation_history() const;

    void print_subturns_history() const;

protected:

    shared_ptr<PSLogger> m_logger;
    static const string m_engine_log_cat;

    void load_level_internal(int p_level_idx);

    optional<vector<PSEngine::SubturnHistory>> next_turn();

    bool next_subturn();

    void apply_rule(const CompiledGame::Rule& p_rule);

    void apply_delta(const RuleApplicationDelta& p_delta);

    bool resolve_movements();

    bool basic_movement_resolution();

    bool try_to_move_object(Cell& p_containing_cell, shared_ptr<CompiledGame::PrimaryObject> p_type_of_object_moved, RuleDelta& p_movement_deltas);

    bool advanced_movement_resolution();

    bool check_win_conditions();
    bool check_win_condition(const CompiledGame::WinCondition& p_win_condition);

    vector<PatternMatchInformation> match_pattern(const CompiledGame::Pattern& p_pattern, AbsoluteDirection p_rule_application_direction);

    bool does_rule_cell_matches_cell(const CompiledGame::CellRule& p_rule_cell, const Cell* p_cell, AbsoluteDirection p_rule_application_direction);

    set<AbsoluteDirection> get_absolute_directions_from_rule_direction(CompiledGame::RuleDirection p_rule_direction);

    optional<set<ObjectMoveType>> convert_entity_rule_info_to_allowed_move_types(CompiledGame::EntityRuleInfo p_entity_rule_info, AbsoluteDirection p_rule_app_dir);
    bool does_move_info_matches_rule(ObjectMoveType p_move_type, CompiledGame::EntityRuleInfo p_rule_info, AbsoluteDirection p_rule_dir);

    bool get_move_destination_coord(int p_origin_x, int p_origin_y, ObjectMoveType p_move_type, int& p_out_dest_x, int& p_out_dest_y);

    RuleApplicationDelta translate_rule_delta(const CompiledGame::Rule& p_rule, AbsoluteDirection p_rule_app_dir, const vector<PatternMatchInformation>& p_pattern_match_infos);

    Cell* get_cell_from(int p_origin_x, int p_origin_y, int p_distance, AbsoluteDirection p_direction);
    Cell* get_cell_at(int p_x, int p_y);


    string get_single_char_obj_alias(const string& p_obj_id);

    CompiledGame m_compiled_game;

    Level m_current_level;

    vector<Level> m_level_state_stack;

    vector<Operation> m_operation_history;

    //todo subturnHistory does not contain information about mouvement that happened or failed during the subturn!!
    vector<SubturnHistory> m_turn_history;

    InputType m_last_input = InputType::None;

    map<string,string> m_single_char_obj_alias_cache;

    bool m_is_level_won = false;

    float m_current_tick_time_elapsed = 0;

    Config m_config;
};
