
#include "CompiledGame.hpp"

#include <iostream>
#include <memory>

#include "EnumHelpers.hpp"

using namespace std;


map<string,CompiledGame::CommandType, ci_less> CompiledGame::to_command_type ={
    {"None", CommandType::None},
	{"Win", CommandType::Win},
	{"Cancel", CommandType::Cancel},
	{"Again", CommandType::Again},
};

map<string,CompiledGame::RuleDirection, ci_less> CompiledGame::to_rule_direction = {
	{"None", RuleDirection::None},
	{"Up", RuleDirection::Up},
	{"Down", RuleDirection::Down},
	{"Left", RuleDirection::Left},
    {"Right", RuleDirection::Right},
    {"Horizontal", RuleDirection::Horizontal},
    {"Vertical", RuleDirection::Vertical},
};

map<string,CompiledGame::EntityRuleInfo, ci_less> CompiledGame::to_entity_rule_info = {
	{"", EntityRuleInfo::None},
	{"Up", EntityRuleInfo::Up},
	{"Down", EntityRuleInfo::Down},
	{"Left", EntityRuleInfo::Left},
    {"Right", EntityRuleInfo::Right},
    {"Moving", EntityRuleInfo::Moving},
    {"Stationary", EntityRuleInfo::Stationary},
    {"Horizontal", EntityRuleInfo::Horizontal},
    {"Vertical", EntityRuleInfo::Vertical},
    {"Orthogonal", EntityRuleInfo::Orthogonal},
    {"Action", EntityRuleInfo::Action},
    {"^", EntityRuleInfo::RelativeUp},
	{"v", EntityRuleInfo::RelativeDown},
	{"<", EntityRuleInfo::RelativeLeft},
    {">", EntityRuleInfo::RelativeRight},
    {"Parallel", EntityRuleInfo::Parallel},
    {"Perpendicular", EntityRuleInfo::Perpendicular},
    {"No", EntityRuleInfo::No},
};

map<string,CompiledGame::WinConditionType, ci_less> CompiledGame::to_win_condition_type = {
    {"None", WinConditionType::None},
	{"All", WinConditionType::All},
	{"Some", WinConditionType::Some},
	{"No", WinConditionType::No},
};

void CompiledGame::PrimaryObject::GetAllPrimaryObjects(vector<weak_ptr<PrimaryObject>>& p_objects, bool only_get_unique_objects /* = true*/) {
    if(only_get_unique_objects)
    {
        for(const weak_ptr<PrimaryObject>&  cur_obj: p_objects)
        {
            if(cur_obj.lock() == shared_from_this())
            {
                return;
            }
        }
    }

    p_objects.push_back(static_pointer_cast<PrimaryObject>(this->shared_from_this()));
}

void CompiledGame::GroupObject::GetAllPrimaryObjects(vector<weak_ptr<PrimaryObject>>& p_objects, bool only_get_unique_objects /* = true*/) {
    for(const weak_ptr<Object>&  cur_obj: referenced_objects)
    {
        cur_obj.lock()->GetAllPrimaryObjects(p_objects,only_get_unique_objects);
    }
}

string CompiledGame::GroupObject::to_string() const{
    string result = Object::to_string() + " refs : ";
    for(weak_ptr<CompiledGame::Object> ref_obj : referenced_objects)
    {
        result += ref_obj.lock()->identifier;
        result += ", ";
    }
    return result;
}

bool CompiledGame::PropertiesObject::defines(const shared_ptr<PrimaryObject>& p_obj) const
{
    for(const weak_ptr<Object>&  cur_obj: referenced_objects)
    {
        if( cur_obj.lock()->defines(p_obj))
        {
            return true;
        }
    }
    return false;
}

bool CompiledGame::CollisionLayer::is_object_on_layer(weak_ptr<PrimaryObject> p_object) const
{
    for(const weak_ptr<PrimaryObject>&  cur_obj: objects)
    {
        if( cur_obj.lock() == p_object.lock())
        {
            return true;
        }
    }
    return false;
}

string CompiledGame::Rule::to_string() const
{
    string result = "";

    result += "(l." + std::to_string(rule_line) + ") ";
    if(is_late_rule)
    {
        result += "late ";
    }
    
    result += enum_to_str(direction,to_rule_direction).value_or("Error") + " ";

    result += "[";
    bool first_time = true;
    for(const auto& cell : match_pattern.cells)
    {
        if(!first_time)
        {
            result += " | ";
        }
        else
        {
            first_time = false;
        }
        
        for(const auto& cell_content : cell.content)
        {
            result += enum_to_str(cell_content.second,to_entity_rule_info).value_or("Error") + " " + cell_content.first->identifier +" ";
        }
    }
    result += "] -> [";
    first_time = true;
    for(const auto& cell : result_pattern.cells)
    {
        if(!first_time)
        {
            result += " | ";
        }
        else
        {
            first_time = false;
        }

        for(const auto& cell_content : cell.content)
        {
            result += enum_to_str(cell_content.second,to_entity_rule_info).value_or("Error") + " "  + cell_content.first->identifier + " " ;
        }
    }
    result += "] ";

    for(const auto& command : commands)
    {
        result += enum_to_str(command,to_command_type).value_or("Error") + " ";
    }

    return result;
}

void CompiledGame::print()
{
    bool p_print_objects = true;
    bool p_print_collision_layers = true;
    bool p_print_rules = true;
    bool p_print_win_conditions = true;
    bool p_print_levels = true;

    if(p_print_objects)
    {
        cout << "Compiled objects :\n";
        for(const auto& obj : objects)
        {
            cout << obj->to_string() << "\n";
        }
    }

    if(p_print_collision_layers)
    {
        cout << "Compiled collision layers :\n";

        for(const auto& col_layer : collision_layers)
        {
            for(const auto& obj : col_layer->objects )
            {
                cout << obj.lock()->identifier << ", ";
            }
            cout << "\n";
        }
    }

    if(p_print_rules)
    {
        cout << "Compiled Rules :\n";
        for(const auto& rule : rules)
        {
            cout << rule.to_string() << "\n";
        }
        cout << "Compiled late Rules :\n";
        for(const auto& rule : late_rules)
        {
            cout << rule.to_string() << "\n";
        }
    }

    if(p_print_win_conditions)
    {
        cout << "Win Conditions :\n";
        for(const auto& condition : win_conditions)
        {
            
            cout << enum_to_str(condition.type,to_win_condition_type).value_or("ERROR") << " ";
            cout << (condition.object != nullptr ? condition.object->identifier : "ERROR" ) << " ";
            cout << (condition.on_object != nullptr ? ("On " + condition.on_object->identifier): "" ) << "\n";
        }
    }

    if(p_print_levels)
    {
        cout << "Compiled levels :\n";

        for(const auto& level : levels)
        {
            cout << "width : " << level.width << " height : "<< level.height << "\n";
            int x = 0;
            for(const auto& cell : level.cells )
            {
                for(const auto& obj : cell.objects )
                {
                    cout << obj.lock()->identifier << ", ";
                }
                ++x;
                if( x == level.width)
                {
                    x = 0;
                    cout << "\n";
                }
                else
                {
                    cout << "|| ";
                }
                
            }
            cout << "\n";
        }
    }
}