#pragma once

#include <string>
#include <map>
#include <unordered_set>

#include "EnumHelpers.hpp"
#include "CompiledGame.hpp"
#include "PSLogger.hpp"
#include "PSUtils.hpp"


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
        PSVector2i position;
        map<shared_ptr<CompiledGame::PrimaryObject>,ObjectMoveType> objects;

        optional<shared_ptr<CompiledGame::PrimaryObject>> find_colliding_object(shared_ptr<CompiledGame::PrimaryObject> obj) const;
    };

    struct Level
    {
        int level_idx = -1;
        PSVector2i size;
        vector<Cell> cells;
    };

    //todo we should probably encapsulate that in the level class
    class ObjectCache{
    public:
        void build_cache(const Level p_level);
        void add_object_position(const shared_ptr<CompiledGame::PrimaryObject> p_object,PSVector2i p_position);
        void remove_object_position(const shared_ptr<CompiledGame::PrimaryObject> p_object,PSVector2i p_position);
        unordered_set<PSVector2i> get_object_positions(const shared_ptr<CompiledGame::PrimaryObject> p_object);
        string to_string();
    private:
        map<const shared_ptr<CompiledGame::PrimaryObject>,unordered_set<PSVector2i>> m_content;
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
        PSVector2i origin;
    };

    struct ObjectDelta
    {
        PSVector2i cell_position;
        shared_ptr<CompiledGame::PrimaryObject> object;
        CompiledGame::ObjectDeltaType type;

        ObjectDelta(PSVector2i p_cell_position, shared_ptr<CompiledGame::PrimaryObject> p_object, CompiledGame::ObjectDeltaType p_type)
        :cell_position(p_cell_position), object(p_object), type(p_type){};

        friend bool operator==(const ObjectDelta& lhs, const ObjectDelta& rhs){
            bool result = lhs.object == rhs.object;
            result &= lhs.type == rhs.type;
            result &= lhs.cell_position == rhs.cell_position;
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
        PSVector2i origin;
        PSVector2i destination;
        AbsoluteDirection move_direction;
        shared_ptr<CompiledGame::PrimaryObject> object;
        bool moved_successfully = false;
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

        vector<CompiledGame::Command> gather_all_subturn_commands() const;
    };

    struct TurnHistory
    {
        vector<SubturnHistory> subturns;

        bool was_turn_cancelled = false;
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
    PSEngine(Config p_config, shared_ptr<PSLogger> p_logger = nullptr);

    void load_game(const CompiledGame& p_game_to_load);

    void Load_first_level();

    void load_level(int p_level_idx);

    optional<TurnHistory> receive_input(InputType p_input);

    optional<TurnHistory> tick(float p_delta_time);

    void restart_level();
    bool undo();

    void load_next_level();

    bool is_level_won() const;

    int get_number_of_levels() const {return (int)m_compiled_game.levels.size();}

    vector<string> get_messages_before_level(int p_level_idx) const {return m_compiled_game.levels_messages[p_level_idx];}
    vector<string> get_messages_after_level(int p_level_idx) const {return m_compiled_game.levels_messages[p_level_idx+1];}

    void print_game_state(); //todo making this const will require a few changes and maybe some mutables

    Level get_level_state() const {return m_current_level;};

    TurnHistory get_turn_deltas() {return m_turn_history;}

    string operation_history_to_string() const;
    void print_operation_history() const;

    void print_subturns_history() const;

protected:

    Config m_config;

    shared_ptr<PSLogger> m_logger;
    static const string m_engine_log_cat;

    void load_level_internal(int p_level_idx);

    optional<TurnHistory> next_turn();

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

    bool get_move_destination_coord(PSVector2i p_origin, ObjectMoveType p_move_type, PSVector2i& p_out_destination);

    RuleApplicationDelta translate_rule_delta(const CompiledGame::Rule& p_rule, AbsoluteDirection p_rule_app_dir, const vector<PatternMatchInformation>& p_pattern_match_infos);

    Cell* get_cell_from(PSVector2i p_origin, int p_distance, AbsoluteDirection p_direction);
    Cell* get_cell_at(PSVector2i p_position);


    string get_single_char_obj_alias(const string& p_obj_id);

    CompiledGame m_compiled_game;

    Level m_current_level;

    vector<Level> m_level_state_stack;

    vector<Operation> m_operation_history;

    TurnHistory m_turn_history;

    InputType m_last_input = InputType::None;

    map<string,string> m_single_char_obj_alias_cache;

    bool m_is_level_won = false;

    float m_current_tick_time_elapsed = 0;

    ObjectCache m_object_cache;
};
