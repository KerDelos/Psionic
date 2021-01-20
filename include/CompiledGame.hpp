#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <algorithm>

#include "EnumHelpers.hpp"

using namespace std;


struct CompiledGame{

    struct PreludeInfo
    {
        optional<string> title;
        optional<string> author;
        optional<string> homepage;
        optional<float> realtime_interval;
    };

    struct AliasObject;
    struct PrimaryObject;
    struct CollisionLayer;

    struct Object : std::enable_shared_from_this<Object>
    {
        string identifier;

        Object(string p_id):identifier(p_id){};
        virtual ~Object(){};

        virtual string to_string() const {return " id : " + identifier;}
        virtual bool is_properties() const {return false;};
        virtual bool is_aggregate() const {return false;};
        virtual shared_ptr<AliasObject> as_alias_object() {return nullptr;}
        virtual shared_ptr<PrimaryObject> as_primary_object() {return nullptr;}
        virtual bool defines(const shared_ptr<PrimaryObject>& p_obj) const = 0;
        virtual void GetAllPrimaryObjects(vector<weak_ptr<PrimaryObject>>& p_objects, bool only_get_unique_objects = true ){};
    };

    struct AliasObject : public Object
    {
        weak_ptr<Object> referenced_object;

        AliasObject(string p_id, weak_ptr<Object>p_obj ):Object(p_id),referenced_object(p_obj){}
        virtual ~AliasObject() override {};

        virtual string to_string() const override {return "(AliasObj)" + Object::to_string() + " ref : "+ referenced_object.lock()->identifier;}
        virtual bool is_properties()const override {return referenced_object.lock()->is_properties();};
        virtual bool is_aggregate() const override {return referenced_object.lock()->is_aggregate();};
        virtual shared_ptr<AliasObject> as_alias_object() override {return static_pointer_cast<AliasObject>(this->shared_from_this());}
        virtual bool defines(const shared_ptr<PrimaryObject>& p_obj) const override {return referenced_object.lock()->defines(p_obj);};
        virtual void GetAllPrimaryObjects(vector<weak_ptr<PrimaryObject>>& p_objects, bool only_get_unique_objects = true ) override {
            referenced_object.lock()->GetAllPrimaryObjects(p_objects,only_get_unique_objects);
        }
    };

    struct PrimaryObject : public Object
    {
        weak_ptr<CollisionLayer> collision_layer;

        PrimaryObject(string p_id):Object(p_id){}
        virtual ~PrimaryObject() override {};

        virtual string to_string() const override {return "(PrimaryObj)" + Object::to_string();}
        virtual shared_ptr<PrimaryObject> as_primary_object() override {return static_pointer_cast<PrimaryObject>(this->shared_from_this());}
        virtual bool defines(const shared_ptr<PrimaryObject>& p_obj) const override {return this == p_obj.get();};
        virtual void GetAllPrimaryObjects(vector<weak_ptr<PrimaryObject>>& p_objects, bool only_get_unique_objects = true ) override;
    };



    struct GroupObject : public Object
    {
        vector<weak_ptr< Object>> referenced_objects;

        GroupObject(string p_id):Object(p_id){}
        virtual ~GroupObject() override {};

        virtual string to_string() const override;
        virtual void GetAllPrimaryObjects(vector<weak_ptr<PrimaryObject>>& p_objects, bool only_get_unique_objects = true ) override;
    };

    struct AggregateObject : public GroupObject
    {
        AggregateObject(string p_id):GroupObject(p_id){}
        virtual ~AggregateObject() override {};

        virtual string to_string() const override {return "(AggregateObj)" + GroupObject::to_string();}
        virtual bool is_aggregate() const override {return true;};
        virtual bool defines(const shared_ptr<PrimaryObject>& p_obj) const override {return false;};

    };

    struct PropertiesObject : public GroupObject
    {
        PropertiesObject(string p_id):GroupObject(p_id){}
        virtual ~PropertiesObject() override {};

        virtual string to_string() const override {return "(PropertiesObj)" + GroupObject::to_string();}
        virtual bool is_properties() const override {return true;};
        virtual bool defines(const shared_ptr<PrimaryObject>& p_obj) const override;
    };

    struct CollisionLayer
    {
        vector<weak_ptr<PrimaryObject>> objects;

        bool is_object_on_layer(weak_ptr<PrimaryObject> p_object) const;
    };

    //enum direction
    enum class RuleDirection{
        None,
        Up,
        Down,
        Left,
        Right,
        Horizontal,
        Vertical
    };

    enum class EntityRuleInfo{
        None,
        Up,
        Down,
        Left,
        Right,
        Moving,
        Stationary,
        Horizontal,
        Vertical,
        Orthogonal,
        Action,
        RelativeUp,
        RelativeDown,
        RelativeLeft,
        RelativeRight,
        Parallel,
        Perpendicular,
        No,
    };

    enum class WinConditionType
    {
        None,
        All,
        Some,
        No,
    };

    enum class CommandType
    {
        None,
        Win,
        Cancel,
        Again,
        Message,
    };

    struct Color
    {
        enum class ColorName{
            None,
            Black,
            White,
            LightGray,
            Gray,
            DarkGray,
            Grey,
            Red,
            DarkRed,
            LightRed,
            Brown,
            DarkBrown,
            LightBrown,
            Orange,
            Yellow,
            Green,
            DarkGreen,
            LightGreen,
            Blue,
            LightBlue,
            DarkBlue,
            Purple,
            Pink,
            Transparent,
        };

        ColorName name;
        string hexcode; //todo change the format
    };


    enum class ObjectDeltaType
    {
        None,
        Appear,
        Disappear,
        Up,
        Down,
        Left,
        Right,
        RelativeUp,
        RelativeDown,
        RelativeLeft,
        RelativeRight,
        Stationary,
        Action,
    };

    static map<string,CommandType, ci_less> to_command_type;
    static map<string,RuleDirection, ci_less> to_rule_direction;
    static map<string,EntityRuleInfo, ci_less> to_entity_rule_info;
    static map<string,WinConditionType, ci_less> to_win_condition_type;
    static map<string,Color::ColorName, ci_less> to_color_name;
    static map<string,ObjectDeltaType, ci_less> to_object_delta_type;

    struct CellRule
    {
        bool is_wildcard_cell = false;

        map<shared_ptr<Object>,EntityRuleInfo> content;
    };

    struct Pattern
    {
        vector<CellRule> cells;
    };

    struct Delta
    {
        Delta(int p_pattern_index, int p_delta_match_index, int p_delta_application_index, shared_ptr<Object> p_object, ObjectDeltaType p_delta_type)
        :pattern_index(p_pattern_index),delta_match_index(p_delta_match_index), delta_application_index(p_delta_application_index), object(p_object), delta_type(p_delta_type){}

        int pattern_index = -1;
        int delta_match_index = -1; //to which cell of the pattern should the object be matched
        int delta_application_index; //to which cell is the pattern applied
        //the last two properties exists because in some case (such as [ A | ] -> [ | A] where A is a group)
        //the object must be found in the first cell of the pattern, disappears in the first cell, but appears in the second

        shared_ptr<Object> object;
        ObjectDeltaType delta_type = ObjectDeltaType::None;

        bool is_optional = false;
    };

    struct Command
    {
        CommandType type = CommandType::None;
        string message; //in case it's a message command;
    };

    struct Rule
    {
        int rule_line = -1;

        bool is_late_rule = false;
        RuleDirection direction = RuleDirection::None;

        vector<Pattern> match_patterns;
        vector<Pattern> result_patterns;

        vector<Command> commands;

        vector<Delta> deltas;

        string to_string(bool with_deltas =false) const;
    };

    struct RuleGroup
    {
        vector<Rule> rules;
    };



    struct WinCondition
    {
        WinConditionType type = WinConditionType::None;
        shared_ptr<Object> object;
        shared_ptr<Object> on_object;
    };

    struct Cell
    {
        vector<weak_ptr<PrimaryObject>> objects;
    };

    struct Level
    {
        int width = -1;
        int height = -1;
        vector<Cell> cells;
    };


    struct ObjectGraphicData
    {
        vector<Color> colors;
        vector<int> pixels;
    };

    PreludeInfo prelude_info;
    map<shared_ptr<PrimaryObject>,ObjectGraphicData> graphics_data;
    set<shared_ptr<Object>> objects;
    weak_ptr<Object> player_object;
    vector<shared_ptr<CollisionLayer>> collision_layers;
    vector<Rule> rules;
    vector<Rule> late_rules;
    vector<WinCondition> win_conditions;
    vector<Level> levels;
    vector<vector<string>> levels_messages;

    void print();
};
