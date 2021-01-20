#include <vector>
#include <algorithm>
#include <assert.h>

#include "Compiler.hpp"

const string Compiler::m_compiler_log_cat = "compiler";

Compiler::Compiler(shared_ptr<PSLogger> p_logger) : m_logger(p_logger)
{
    if(m_logger == nullptr)
	{
		cout << "A Logger ptr was not passed to the compiler constructor, construction a default one\n";
		m_logger = make_shared<PSLogger>(PSLogger());
	}
}

std::optional<CompiledGame> Compiler::compile_game(const ParsedGame& p_parsed_game)
{
    m_compiled_game = CompiledGame();

    m_logger->log(PSLogger::LogType::Log, m_compiler_log_cat, "Compiling");

    compile_prelude(p_parsed_game.prelude_tokens);
    compile_objects(p_parsed_game.objects_tokens);
    compile_legend(p_parsed_game.legend_tokens);

    retrieve_background_objects();
    retrieve_player_object();

    compile_collision_layers(p_parsed_game.collision_layers_tokens);
    compile_rules(p_parsed_game.rules_tokens);
    compile_win_conditions(p_parsed_game.win_conditions_tokens);
    compile_levels(p_parsed_game.levels_tokens);

    reference_collision_layers_in_objects();

    verify_rules_and_compute_deltas(m_compiled_game.rules);
    verify_rules_and_compute_deltas(m_compiled_game.late_rules);

    m_logger->log(PSLogger::LogType::Log, m_compiler_log_cat, "Finished compiling");

    return m_has_error ? nullopt : std::optional<CompiledGame>(m_compiled_game);
}

bool Compiler::retrieve_player_object()
{
    bool found_player_obj = false;
    for(shared_ptr<CompiledGame::Object> obj : m_compiled_game.objects)
    {
        if(obj->identifier == "Player")
        {
            if(obj->is_aggregate())
            {
                detect_error(0,"found a Player object but it's an aggregate object which is not allowed for the player object.");
                return false;
            }

            m_compiled_game.player_object = obj;
            found_player_obj = true;
        }
    }

    if(!found_player_obj)
    {
        detect_error(0,"Could not find Player obj in game");
        return false;
    }

    return true;
}

bool Compiler::retrieve_background_objects()
{
    ci_equal case_unsensitive_equal;

    for(shared_ptr<CompiledGame::Object> obj : m_compiled_game.objects)
    {
        if(case_unsensitive_equal(obj->identifier,"Background"))
        {
            m_background_object = obj;
            break;
        }
    }

    if(m_background_object.lock() == nullptr)
    {
        detect_error(0,"Background object was not found in the list of objects.");
        return false;
    }

    if(m_background_object.lock()->is_aggregate())
    {
        detect_error(0,"Background object was found but is an aggregate. background object can only be a primary object or a property object.");
        return false;
    }

    vector<weak_ptr<CompiledGame::PrimaryObject>> prim_objs;
    m_background_object.lock()->GetAllPrimaryObjects(prim_objs);
    m_default_background_object = prim_objs[0].lock();

    return true;
}

void Compiler::compile_prelude(const vector<Token<ParsedGame::PreludeTokenType>>& p_prelude_tokens)
{
    m_logger->log(PSLogger::LogType::Log, m_compiler_log_cat, "Compiling Prelude");

    ParsedGame::PreludeTokenType last_token = ParsedGame::PreludeTokenType::None;

    for(auto& token : p_prelude_tokens)
    {
        switch (token.token_type)
        {
        case ParsedGame::PreludeTokenType::Title:
        case ParsedGame::PreludeTokenType::Author:
        case ParsedGame::PreludeTokenType::Homepage:
        case ParsedGame::PreludeTokenType::RealtimeInterval:
            if(last_token != ParsedGame::PreludeTokenType::None)
            {
                detect_error(token,"Unexpected token "+ enum_to_str(token.token_type, ParsedGame::to_prelude_token_type).value_or("ERROR") + " here.");
                continue;
            }
            else
            {
                last_token = token.token_type;
            }
            break;

        case ParsedGame::PreludeTokenType::Literal:
            if(last_token == ParsedGame::PreludeTokenType::None)
            {
                detect_error(token,"Unexpected literal "+ token.str_value + " here.");
                continue;
            }

            if(last_token == ParsedGame::PreludeTokenType::Title)
            {
                m_compiled_game.prelude_info.title = token.str_value;
            }
            else if(last_token == ParsedGame::PreludeTokenType::Author)
            {
                m_compiled_game.prelude_info.author = token.str_value;
            }
            else if(last_token == ParsedGame::PreludeTokenType::Homepage)
            {
                m_compiled_game.prelude_info.homepage = token.str_value;
            }
            else if(last_token == ParsedGame::PreludeTokenType::RealtimeInterval)
            {
                try
                {
                    m_compiled_game.prelude_info.realtime_interval = stof(token.str_value);
                }
                catch(const std::exception& e)
                {
                   detect_error(token, "was expecting a float value after realtime_interval but found \"" +token.str_value+ "\".");
                }
            }
            else
            {
                detect_error(token,"Internal error, probably missing a case in the prelude compiler for \"" +enum_to_str(last_token, ParsedGame::to_prelude_token_type).value_or("ERROR")+ "\" associated literal.");
            }


            last_token = ParsedGame::PreludeTokenType::None;

            break;

        case ParsedGame::PreludeTokenType::None:
        default:
            detect_error(token,"Unexpected prelude token ("+enum_to_str(token.token_type, ParsedGame::to_prelude_token_type).value_or("ERROR")+":"+ token.str_value +").");
            break;
        }
    }

    m_logger->log(PSLogger::LogType::Log, m_compiler_log_cat, "Finished Compiling Prelude");
}


//todo compile color and drawing information
void Compiler::compile_objects(const vector<Token<ParsedGame::ObjectsTokenType>>& p_objects_tokens)
{
    enum class ObjectCompilingState{
        WaitingForIdentifier,
        WaitingForColor,
        WaitingForColorOrPixelOrIdentifier,
        WaitingForPixel,
    };

    const int PIXEL_NUMBER = 25;

    m_logger->log(PSLogger::LogType::Log, m_compiler_log_cat, "Compiling Objects");

    ObjectCompilingState state = ObjectCompilingState::WaitingForIdentifier;

    shared_ptr<CompiledGame::PrimaryObject> last_object;
    CompiledGame::ObjectGraphicData graphic_data;

    for(auto& token : p_objects_tokens)
    {
        switch(state)
        {
            case ObjectCompilingState::WaitingForIdentifier:
                if(token.token_type == ParsedGame::ObjectsTokenType::Literal)
                {
                    if(!check_identifier_validity(token.str_value, token.token_line, false))
                    {
                        detect_error(token, "invalid identifier");
                        break;
                    }

                    shared_ptr<CompiledGame::PrimaryObject> obj( new CompiledGame::PrimaryObject(token.str_value));
                    m_compiled_game.objects.insert(obj);
                    last_object = obj;
                    state = ObjectCompilingState::WaitingForColor;
                }
                else
                {
                    detect_error(token, "was expecting identifier");
                }

            break;
            //----------------------------------

            case ObjectCompilingState::WaitingForColor:
                if(token.token_type == ParsedGame::ObjectsTokenType::ColorHexCode)
                {
                    CompiledGame::Color color;
                    color.hexcode = token.str_value;
                    state = ObjectCompilingState::WaitingForColorOrPixelOrIdentifier;
                    graphic_data.colors.push_back(color);
                }
                else if(token.token_type == ParsedGame::ObjectsTokenType::ColorName)
                {
                    CompiledGame::Color color;
                    color.name = str_to_enum(token.str_value, CompiledGame::to_color_name).value_or(CompiledGame::Color::ColorName::None);
                    if(color.name == CompiledGame::Color::ColorName::None)
                    {
                        detect_error(token,"invalid color name");
                    }
                    state = ObjectCompilingState::WaitingForColorOrPixelOrIdentifier;
                    graphic_data.colors.push_back(color);
                }
                else
                {
                    detect_error(token, "was expecting a color name or hexcode");
                }

            break;
            //----------------------------------

            case ObjectCompilingState::WaitingForColorOrPixelOrIdentifier:
                if(token.token_type == ParsedGame::ObjectsTokenType::ColorHexCode)
                {
                    CompiledGame::Color color;
                    color.hexcode = token.str_value;
                    state = ObjectCompilingState::WaitingForColorOrPixelOrIdentifier;
                    graphic_data.colors.push_back(color);
                }
                else if(token.token_type == ParsedGame::ObjectsTokenType::ColorName)
                {
                    CompiledGame::Color color;
                    color.name = str_to_enum(token.str_value, CompiledGame::to_color_name).value_or(CompiledGame::Color::ColorName::None);
                    if(color.name == CompiledGame::Color::ColorName::None)
                    {
                        detect_error(token,"invalid color name");
                    }
                    state = ObjectCompilingState::WaitingForColorOrPixelOrIdentifier;
                    graphic_data.colors.push_back(color);
                }
                else if(token.token_type == ParsedGame::ObjectsTokenType::Literal)
                {
                    if(graphic_data.pixels.size() == PIXEL_NUMBER || graphic_data.pixels.size() == 0)
                    {
                        m_compiled_game.graphics_data[last_object] = graphic_data;
                        graphic_data = CompiledGame::ObjectGraphicData();
                    }
                    else
                    {
                        detect_error(token,"too many or too few pixels");
                        break;
                    }

                    if(!check_identifier_validity(token.str_value, token.token_line, false))
                    {
                        detect_error(token, "invalid identifier");
                        break;
                    }

                    shared_ptr<CompiledGame::PrimaryObject> obj( new CompiledGame::PrimaryObject(token.str_value));
                    m_compiled_game.objects.insert(obj);
                    last_object = obj;
                    state = ObjectCompilingState::WaitingForColor;

                }
                else if(token.token_type == ParsedGame::ObjectsTokenType::Pixel)
                {
                    if(token.str_value == ".")
                    {
                        graphic_data.pixels.push_back(-1); //-1 stands for transparent
                    }
                    else
                    {
                        int pixel_value = -1;
                        try
                        {
                            pixel_value = stoi(token.str_value);
                        }
                        catch(...)
                        {
                            detect_error(token, "improper pixel value");
                            break;
                        }
                        graphic_data.pixels.push_back(pixel_value);
                    }

                    state = ObjectCompilingState::WaitingForPixel;
                }
                else
                {
                    detect_error(token, "was expecting a color, a pixel or and identifier");
                }
            break;
            //----------------------------------

            case ObjectCompilingState::WaitingForPixel:
                if(token.token_type == ParsedGame::ObjectsTokenType::Pixel)
                {
                    if(token.str_value == ".")
                    {
                        graphic_data.pixels.push_back(-1); //-1 stands for transparent
                    }
                    else
                    {
                        int pixel_value = -1;
                        try
                        {
                            pixel_value = stoi(token.str_value);
                        }
                        catch(...)
                        {
                            detect_error(token, "improper pixel value");
                            break;
                        }
                        graphic_data.pixels.push_back(pixel_value);
                    }

                    if(graphic_data.pixels.size() == 25)
                    {
                        m_compiled_game.graphics_data[last_object] = graphic_data;
                        graphic_data = CompiledGame::ObjectGraphicData();
                        state = ObjectCompilingState::WaitingForIdentifier;
                    }

                }
                else
                {
                    detect_error(token, "was expecting a pixel");
                }
            break;
            //----------------------------------

            default:
                detect_error(token,"should never happen");
            break;
            //----------------------------------
        }
    }

    m_logger->log(PSLogger::LogType::Log, m_compiler_log_cat, "Finished Compiling Objects");
}


void Compiler::compile_legend(const vector<Token<ParsedGame::LegendTokenType>>& p_legend_tokens)
{
    enum class LegendCompilationState{
        None,
        WaitingForIdentifier,
        WaitingForEqual,
        WaitingForObjectReference,
        WaitingForGroupOrReturn,
    };

    enum class ObjectGroupType{
        None,
        Or,
        And,
    };

    LegendCompilationState compilation_state = LegendCompilationState::WaitingForIdentifier;

    m_logger->log(PSLogger::LogType::Log, m_compiler_log_cat, "Compiling Legend");

    string current_identifier = "";
    vector<weak_ptr<CompiledGame::Object>> current_obj_refs;
    ObjectGroupType current_group_type = ObjectGroupType::None;

    for(auto& token : p_legend_tokens)
    {
        //todo we should try to recover from errors but for now I guess it's sufficient
        if(m_has_error) // do not proceed if there is errors
        {
            break;
        }

        switch (compilation_state)
        {
            case LegendCompilationState::WaitingForIdentifier:
                if(token.token_type == ParsedGame::LegendTokenType::Return)
                {
                    //skipping returns as there might be multiple ones
                }
                else if (token.token_type == ParsedGame::LegendTokenType::Identifier)
                {
                    if(check_identifier_validity(token.str_value, token.token_line, false))
                    {
                        current_identifier = token.str_value;
                        compilation_state = LegendCompilationState::WaitingForEqual;
                    }
                }
                else
                {
                    detect_error(token, "Unexpected token here. was expecting an identifier.");
                }
            break;

            case LegendCompilationState::WaitingForEqual:
                if(token.token_type == ParsedGame::LegendTokenType::Equal)
                {
                    compilation_state = LegendCompilationState::WaitingForObjectReference;
                }
                else
                {
                    detect_error(token, "Unexpected token here. was expecting an \"=\".");
                }
            break;

            case LegendCompilationState::WaitingForObjectReference:
                if(token.token_type == ParsedGame::LegendTokenType::Identifier)
                {
                    if(check_identifier_validity(token.str_value, token.token_line, true))
                    {
                        compilation_state = LegendCompilationState::WaitingForGroupOrReturn;
                        current_obj_refs.push_back(get_obj_by_id(token.str_value));
                    }
                }
                else
                {
                    detect_error(token, "Unexpected token here. was expecting an identifier.");
                }
            break;

            case LegendCompilationState::WaitingForGroupOrReturn:
                if(token.token_type == ParsedGame::LegendTokenType::Return)
                {
                    compilation_state = LegendCompilationState::WaitingForIdentifier;

                    switch (current_group_type)
                    {
                    case ObjectGroupType::None:
                        {
                            shared_ptr<CompiledGame::AliasObject> obj( new CompiledGame::AliasObject(current_identifier,current_obj_refs.front()));
                            m_compiled_game.objects.insert(obj);
                        }
                        break;
                    case ObjectGroupType::Or:
                        {
                            shared_ptr<CompiledGame::PropertiesObject> obj( new CompiledGame::PropertiesObject(current_identifier));
                            for(weak_ptr<CompiledGame::Object> ref_obj : current_obj_refs)
                            {
                                if(ref_obj.lock()->is_aggregate())
                                {
                                    detect_error(token, "cannot mix aggregates identifier in a properties identifier.");
                                    break;
                                }
                                obj->referenced_objects.push_back(ref_obj);
                            }
                            m_compiled_game.objects.insert(obj);
                        }
                        break;
                    case ObjectGroupType::And:
                        {
                            shared_ptr<CompiledGame::AggregateObject> obj( new CompiledGame::AggregateObject(current_identifier));
                            for(weak_ptr<CompiledGame::Object> ref_obj : current_obj_refs)
                            {
                                if(ref_obj.lock()->is_properties())
                                {
                                    detect_error(token, "cannot mix properties identifier in an aggregate identifier.");
                                    break;
                                }
                                obj->referenced_objects.push_back(ref_obj);
                            }
                            m_compiled_game.objects.insert(obj);
                        }
                        break;

                    default:
                        detect_error(token, "this should never happen and suggest a compiler error in Compiler::compile_legend.");
                        break;
                    }

                    //clearing everything to prepare for next object compilation
                    current_identifier = "";
                    current_obj_refs.clear();
                    current_group_type = ObjectGroupType::None;
                }
                else if(token.token_type == ParsedGame::LegendTokenType::And)
                {
                    if(current_group_type == ObjectGroupType::Or)
                    {
                        detect_error(token, "You cannot mix \"and\" and \"or\" keywords in a legend.");
                    }
                    else
                    {
                        compilation_state = LegendCompilationState::WaitingForObjectReference;
                        current_group_type = ObjectGroupType::And;
                    }
                }
                else if(token.token_type == ParsedGame::LegendTokenType::Or)
                {
                    if(current_group_type == ObjectGroupType::And)
                    {
                        detect_error(token, "You cannot mix \"and\" and \"or\" keywords in a legend.");
                    }
                    else
                    {
                        compilation_state = LegendCompilationState::WaitingForObjectReference;
                        current_group_type = ObjectGroupType::Or;
                    }
                }
                else
                {
                    detect_error(token, "Unexpected token here. was expecting an identifier.");
                }
            break;

            default:
            detect_error(token,"Unexpected legend compiler internal state, this should not happen.");
            break;
        }
    }
    m_logger->log(PSLogger::LogType::Log, m_compiler_log_cat, "Finished Compiling Legend");
}

void Compiler::compile_collision_layers(const vector<Token<ParsedGame::CollisionLayersTokenType>>& p_collision_layer_tokens)
{
    shared_ptr<CompiledGame::CollisionLayer> current_collision_layer( new CompiledGame::CollisionLayer());

    m_logger->log(PSLogger::LogType::Log, m_compiler_log_cat, "Compiling Collision Layers");

    for(auto& token : p_collision_layer_tokens)
    {
        switch (token.token_type)
        {
        case ParsedGame::CollisionLayersTokenType::Return:
            if(current_collision_layer->objects.size() > 0)
            {
                m_compiled_game.collision_layers.push_back(current_collision_layer);
                current_collision_layer.reset();
                current_collision_layer = shared_ptr<CompiledGame::CollisionLayer>( new CompiledGame::CollisionLayer());
            }
            break;

        case ParsedGame::CollisionLayersTokenType::Identifier:
            if(check_identifier_validity(token.str_value, token.token_line, true))
            {
                weak_ptr<CompiledGame::Object> reference_object = get_obj_by_id(token.str_value);

                if(reference_object.lock()->is_aggregate())
                {
                    detect_error(token, "Cannot add an aggregate on a collision layer as its composing objects must be in different layers");
                }

                vector<weak_ptr<CompiledGame::PrimaryObject>> primary_objects_referenced;
                reference_object.lock()->GetAllPrimaryObjects(primary_objects_referenced);

                for(weak_ptr<CompiledGame::PrimaryObject>& cur_obj : primary_objects_referenced)
                {
                    //check if the object is not already present in a collision layer -> error
                    for(shared_ptr<CompiledGame::CollisionLayer> collision_layer : m_compiled_game.collision_layers)
                    {
                        if(collision_layer->is_object_on_layer(cur_obj))
                        {
                            detect_error(token, "identifier is present in another collision layer");
                        }
                    }

                    if(!current_collision_layer->is_object_on_layer(cur_obj))
                    {
                        current_collision_layer->objects.push_back(cur_obj);
                    }
                }
            }
            break;

        default:
            break;
        }
    }
    m_logger->log(PSLogger::LogType::Log, m_compiler_log_cat, "Finished Compiling Collision Layers");
}

void Compiler::compile_rules(const vector<Token<ParsedGame::RulesTokenType>>& p_rules_tokens)
{
    m_logger->log(PSLogger::LogType::Log, m_compiler_log_cat, "Compiling Rules");

    typedef ParsedGame::RulesTokenType RulesToken;
    static const vector<RulesToken> commands = {
        RulesToken::AGAIN,
        RulesToken::CHECKPOINT,
        RulesToken::CANCEL,
        RulesToken::WIN,
        RulesToken::MESSAGE,
        RulesToken::SFX0,RulesToken::SFX1,RulesToken::SFX2,RulesToken::SFX3,RulesToken::SFX4,
        RulesToken::SFX5,RulesToken::SFX6,RulesToken::SFX7,RulesToken::SFX8,RulesToken::SFX9,RulesToken::SFX10
    };

    static const map<RulesToken, CompiledGame::RuleDirection> token_to_rule_direction = {
        {RulesToken::HORIZONTAL,CompiledGame::RuleDirection::Horizontal},
        {RulesToken::VERTICAL,CompiledGame::RuleDirection::Vertical},
        {RulesToken::UP,CompiledGame::RuleDirection::Up},
        {RulesToken::DOWN,CompiledGame::RuleDirection::Down},
        {RulesToken::LEFT,CompiledGame::RuleDirection::Left},
        {RulesToken::RIGHT,CompiledGame::RuleDirection::Right},
    };

    static const map<RulesToken, CompiledGame::EntityRuleInfo> token_to_entity_rule_info = {
        {RulesToken::RelativeRight,CompiledGame::EntityRuleInfo::RelativeRight},
        {RulesToken::RelativeLeft,CompiledGame::EntityRuleInfo::RelativeLeft},
        {RulesToken::RelativeUp,CompiledGame::EntityRuleInfo::RelativeUp},
        {RulesToken::RelativeDown,CompiledGame::EntityRuleInfo::RelativeDown},
        {RulesToken::MOVING,CompiledGame::EntityRuleInfo::Moving},
        {RulesToken::STATIONARY,CompiledGame::EntityRuleInfo::Stationary},
        {RulesToken::PARALLEL,CompiledGame::EntityRuleInfo::Parallel},
        {RulesToken::PERPENDICULAR,CompiledGame::EntityRuleInfo::Perpendicular},
        {RulesToken::HORIZONTAL,CompiledGame::EntityRuleInfo::Horizontal},
        {RulesToken::VERTICAL,CompiledGame::EntityRuleInfo::Vertical},
        {RulesToken::ORTHOGONAL,CompiledGame::EntityRuleInfo::Orthogonal},
        {RulesToken::ACTION,CompiledGame::EntityRuleInfo::Action},
        {RulesToken::UP,CompiledGame::EntityRuleInfo::Up},
        {RulesToken::DOWN,CompiledGame::EntityRuleInfo::Down},
        {RulesToken::LEFT,CompiledGame::EntityRuleInfo::Left},
        {RulesToken::RIGHT,CompiledGame::EntityRuleInfo::Right},
        {RulesToken::No,CompiledGame::EntityRuleInfo::No},
    };

    static const vector<RulesToken> syntax_tokens = {
        RulesToken::LATE,
        RulesToken::LeftBracket,
        RulesToken::RightBracket,
        RulesToken::Bar,
        RulesToken::Arrow,
        RulesToken::Dots,
        RulesToken::Return,
    };


    enum class RuleCompilingState{
        WaitingForLateOrDirectionOrLeftPatternStart,
        WaitingForDirectionOrLeftPatternStart,
        WaitingForLeftPatternStart,
        WaitingForLeftPatternObjectModifierOrIdentifierOrSyntax,
        WaitingForLeftPatternObjectIdentifier,
        WaitingForArrowOrLeftPatternStart,
        WaitingForRightPatternStart,
        WaitingForRightPatternObjectModifierOrIdentifierOrSyntax,
        WaitingForRightPatternObjectIdentifier,
        WaitingForCommandOrReturnOrRightPatternStart,
        WaitingForCommandOrReturn,
        WaitingForMessageContentOrReturn,
        WaitingForReturn,
    };

    RuleCompilingState state = RuleCompilingState::WaitingForLateOrDirectionOrLeftPatternStart;

    CompiledGame::Rule current_rule;
    CompiledGame::EntityRuleInfo cached_entity_info = CompiledGame::EntityRuleInfo::None;

    auto reset_temp_data = [&](){
        current_rule = CompiledGame::Rule();
        current_rule.match_patterns.push_back(CompiledGame::Pattern());
        current_rule.result_patterns.push_back(CompiledGame::Pattern());
        current_rule.match_patterns[0].cells.push_back(CompiledGame::CellRule());
        current_rule.result_patterns[0].cells.push_back(CompiledGame::CellRule());

        cached_entity_info = CompiledGame::EntityRuleInfo::None;
    };

    auto accept_return_token = [&](Token<ParsedGame::RulesTokenType> token){
        //todo make a final check on rule, ie, do the pattern match in size and do the ... correspond ?

        current_rule.rule_line = token.token_line;
        if(current_rule.is_late_rule)
        {
            m_compiled_game.late_rules.push_back(current_rule);
        }
        else
        {
            m_compiled_game.rules.push_back(current_rule);
        }

        reset_temp_data();

        state = RuleCompilingState::WaitingForLateOrDirectionOrLeftPatternStart;
    };

    reset_temp_data();

    for(auto& token : p_rules_tokens)
    {
        if(m_has_error)
        {
            break;
        }

        //token.print(ParsedGame::to_rules_token_type);

        switch(state)
        {
            case RuleCompilingState::WaitingForLateOrDirectionOrLeftPatternStart:
                if(token.token_type == RulesToken::Return)
                {
                    //Multiple returns are okay
                }
                else if(token.token_type == RulesToken::LATE)
                {
                    current_rule.is_late_rule = true;

                    state = RuleCompilingState::WaitingForDirectionOrLeftPatternStart;
                }
                else if(token_to_rule_direction.find(token.token_type) != token_to_rule_direction.end())
                {
                    current_rule.direction = token_to_rule_direction.find(token.token_type)->second;

                    state = RuleCompilingState::WaitingForLeftPatternStart;
                }
                else if(token.token_type == RulesToken::LeftBracket)
                {
                    state = RuleCompilingState::WaitingForLeftPatternObjectModifierOrIdentifierOrSyntax;
                }
                else
                {
                    detect_error(token, "expecting Late or [");
                }
            break;
            //------------------------------------------------------------------

            case RuleCompilingState::WaitingForDirectionOrLeftPatternStart:
                if(token_to_rule_direction.find(token.token_type) != token_to_rule_direction.end())
                {
                    current_rule.direction = token_to_rule_direction.find(token.token_type)->second;

                    state = RuleCompilingState::WaitingForLeftPatternStart;
                }
                else if(token.token_type == RulesToken::LeftBracket)
                {
                    state = RuleCompilingState::WaitingForLeftPatternObjectModifierOrIdentifierOrSyntax;
                }
                else
                {
                    detect_error(token, "expecting Late or [");
                }
            break;
            //------------------------------------------------------------------

            case RuleCompilingState::WaitingForLeftPatternStart:
                if(token.token_type == RulesToken::LeftBracket)
                {
                    state = RuleCompilingState::WaitingForLeftPatternObjectModifierOrIdentifierOrSyntax;
                }
                else
                {
                    detect_error(token, "expecting Late or [");
                }
            break;
            //------------------------------------------------------------------

            case RuleCompilingState::WaitingForLeftPatternObjectModifierOrIdentifierOrSyntax:
                if(token.token_type == RulesToken::Dots)
                {
                    const CompiledGame::CellRule& cur_cell = current_rule.match_patterns.back().cells.back();
                    if(cur_cell.is_wildcard_cell)
                    {
                        detect_error(token, "cannot have mutliple \"...\" in the same cell.");
                    }
                    else if(cur_cell.content.size() != 0)
                    {
                        //todo allow it ?
                        //todo no error is detected if an object is added after the ...
                        detect_error(token, "cannot have objects in a \"...\" cell.");
                    }
                    else
                    {
                        current_rule.match_patterns.back().cells.back().is_wildcard_cell = true;
                    }
                }
                else if(token.token_type == RulesToken::Bar)
                {
                    current_rule.match_patterns.back().cells.push_back(CompiledGame::CellRule());
                }
                else if(token.token_type == RulesToken::RightBracket)
                {
                    state = RuleCompilingState::WaitingForArrowOrLeftPatternStart;
                }
                else if(token_to_entity_rule_info.find(token.token_type) != token_to_entity_rule_info.end())
                {
                    cached_entity_info = token_to_entity_rule_info.find(token.token_type)->second;

                    state = RuleCompilingState::WaitingForLeftPatternObjectIdentifier;
                }
                else if(token.token_type == RulesToken::Identifier)
                {
                    if(check_identifier_validity(token.str_value,token.token_line,true))
                    {
                        shared_ptr<CompiledGame::Object> obj_ptr = get_obj_by_id(token.str_value).lock();

                        CompiledGame::CellRule& cur_cel = current_rule.match_patterns.back().cells.back();
                        if( cur_cel.content.find(obj_ptr) != cur_cel.content.end())
                        {
                            detect_error(token, "cannot add twice the same object in a cell.");
                        }
                        else
                        {
                            cur_cel.content[obj_ptr] = cached_entity_info;
                        }

                        cached_entity_info = CompiledGame::EntityRuleInfo::None;
                    }

                    state = RuleCompilingState::WaitingForLeftPatternObjectModifierOrIdentifierOrSyntax;
                }
                else
                {
                    detect_error(token, "was not expecting that.");
                }
            break;
            //------------------------------------------------------------------

            case RuleCompilingState::WaitingForLeftPatternObjectIdentifier:
                if(token.token_type == RulesToken::Identifier)
                {
                    if(check_identifier_validity(token.str_value,token.token_line,true))
                    {
                        weak_ptr<CompiledGame::Object> obj = get_obj_by_id(token.str_value);

                        shared_ptr<CompiledGame::Object> obj_ptr = get_obj_by_id(token.str_value).lock();

                        CompiledGame::CellRule& cur_cel = current_rule.match_patterns.back().cells.back();
                        if( cur_cel.content.find(obj_ptr) != cur_cel.content.end())
                        {
                            detect_error(token, "cannot add twice the same object in a cell.");
                        }
                        else
                        {
                            cur_cel.content[obj_ptr] = cached_entity_info;
                        }

                        cached_entity_info = CompiledGame::EntityRuleInfo::None;
                    }

                    state = RuleCompilingState::WaitingForLeftPatternObjectModifierOrIdentifierOrSyntax;
                }
                else
                {
                    detect_error(token, "was expecting an object identifier.");
                }
            break;
            //------------------------------------------------------------------

            case RuleCompilingState::WaitingForArrowOrLeftPatternStart:
                if(token.token_type == RulesToken::Arrow)
                {
                    state = RuleCompilingState::WaitingForRightPatternStart;
                }
                else if(token.token_type == RulesToken::LeftBracket)
                {
                    state = RuleCompilingState::WaitingForLeftPatternObjectModifierOrIdentifierOrSyntax;
                    current_rule.match_patterns.push_back(CompiledGame::Pattern());
                    current_rule.match_patterns.back().cells.push_back(CompiledGame::CellRule());
                }
                else
                {
                    detect_error(token, "expecting -> or [");
                }
            break;
            //------------------------------------------------------------------

            case RuleCompilingState::WaitingForRightPatternStart:
                if(token.token_type == RulesToken::LeftBracket)
                {
                    state = RuleCompilingState::WaitingForRightPatternObjectModifierOrIdentifierOrSyntax;
                }
                else
                {
                    detect_error(token, "expecting [");
                }
            break;
            //------------------------------------------------------------------

            case RuleCompilingState::WaitingForRightPatternObjectModifierOrIdentifierOrSyntax:
                if(token.token_type == RulesToken::Dots)
                {
                    const CompiledGame::CellRule& cur_cell = current_rule.result_patterns.back().cells.back();
                    if(cur_cell.is_wildcard_cell)
                    {
                        detect_error(token, "cannot have mutliple \"...\" in the same cell.");
                    }
                    else if(cur_cell.content.size() != 0)
                    {
                        detect_error(token, "cannot have objects in a \"...\" cell.");
                    }
                    else
                    {
                        current_rule.result_patterns.back().cells.back().is_wildcard_cell = true;
                    }
                }
                else if(token.token_type == RulesToken::Bar)
                {
                    current_rule.result_patterns.back().cells.push_back(CompiledGame::CellRule());
                }
                else if(token.token_type == RulesToken::RightBracket)
                {
                    state = RuleCompilingState::WaitingForCommandOrReturnOrRightPatternStart;
                }
                else if(token_to_entity_rule_info.find(token.token_type) != token_to_entity_rule_info.end())
                {
                    cached_entity_info = token_to_entity_rule_info.find(token.token_type)->second;

                    state = RuleCompilingState::WaitingForRightPatternObjectIdentifier;
                }
                else if(token.token_type == RulesToken::Identifier)
                {
                    if(check_identifier_validity(token.str_value,token.token_line,true))
                    {
                        shared_ptr<CompiledGame::Object> obj_ptr = get_obj_by_id(token.str_value).lock();

                        CompiledGame::CellRule& cur_cel = current_rule.result_patterns.back().cells.back();
                        if( cur_cel.content.find(obj_ptr) != cur_cel.content.end())
                        {
                            detect_error(token, "cannot add twice the same object in a cell.");
                        }
                        else
                        {
                            cur_cel.content[obj_ptr] = cached_entity_info;
                        }

                        cached_entity_info = CompiledGame::EntityRuleInfo::None;
                    }

                    state = RuleCompilingState::WaitingForRightPatternObjectModifierOrIdentifierOrSyntax;
                }
                else
                {
                    detect_error(token, "was not expecting that.");
                }
            break;
            //------------------------------------------------------------------

            case RuleCompilingState::WaitingForRightPatternObjectIdentifier:
                if(token.token_type == RulesToken::Identifier)
                {
                    if(check_identifier_validity(token.str_value,token.token_line,true))
                    {
                        shared_ptr<CompiledGame::Object> obj_ptr = get_obj_by_id(token.str_value).lock();

                        CompiledGame::CellRule& cur_cel = current_rule.result_patterns.back().cells.back();
                        if( cur_cel.content.find(obj_ptr) != cur_cel.content.end())
                        {
                            detect_error(token, "cannot add twice the same object in a cell.");
                        }
                        else
                        {
                            cur_cel.content[obj_ptr] = cached_entity_info;
                        }

                        cached_entity_info = CompiledGame::EntityRuleInfo::None;
                    }

                    state = RuleCompilingState::WaitingForRightPatternObjectModifierOrIdentifierOrSyntax;
                }
                else
                {
                    detect_error(token, "was expecting and object identifier.");
                }
            break;
            //------------------------------------------------------------------

            case RuleCompilingState::WaitingForCommandOrReturnOrRightPatternStart:
                if(token.token_type == RulesToken::LeftBracket)
                {
                    current_rule.result_patterns.push_back(CompiledGame::Pattern());
                    current_rule.result_patterns.back().cells.push_back(CompiledGame::CellRule());
                    state = RuleCompilingState::WaitingForRightPatternObjectModifierOrIdentifierOrSyntax;
                    break;
                }

            //!!!! INTENTIONAL FALLTHROUGH !!!!

            case RuleCompilingState::WaitingForCommandOrReturn:
                if(token.token_type == RulesToken::Return)
                {
                    accept_return_token(token);
                }
                else if(token.token_type == RulesToken::MESSAGE)
                {
                    //the command will be added to the rule once we parse a message content or a return
                    state = RuleCompilingState::WaitingForMessageContentOrReturn;
                }
                else if(find(commands.begin(),commands.end(), token.token_type) != commands.end())
                {
                    CompiledGame::Command command;
                    command.type = str_to_enum(enum_to_str(token.token_type, ParsedGame::to_rules_token_type).value_or("ERROR"),CompiledGame::to_command_type).value_or(CompiledGame::CommandType::None);
                    current_rule.commands.push_back(command);
                }
                else
                {
                    detect_error(token, "expecting command or return.");
                }
            break;
            //------------------------------------------------------------------

            case RuleCompilingState::WaitingForMessageContentOrReturn:
            {
                CompiledGame::Command command;
                command.type = CompiledGame::CommandType::Message;

                if(token.token_type == RulesToken::MessageContent)
                {
                    command.message = token.str_value;
                    state = RuleCompilingState::WaitingForReturn;
                }
                else if(token.token_type == RulesToken::Return)
                {
                    accept_return_token(token);
                }
                else
                {
                    assert(false);
                    //expecting MessageContent or Return, anything else would probably mean an error in the parser or the compiler
                }
                current_rule.commands.push_back(command);
            }
            break;
            //------------------------------------------------------------------

            case RuleCompilingState::WaitingForReturn:
                assert(token.token_type == RulesToken::Return);
                //expecting Return, anything else would probably mean an error in the parser or the compiler
                accept_return_token(token);
            break;
            //------------------------------------------------------------------

            default:
                detect_error(token, "wrong compiler state in rule compiling. this is an implementation error");
            break;
        }

    }
    m_logger->log(PSLogger::LogType::Log, m_compiler_log_cat, "Finished Compiling Rules");
}

void Compiler::compile_win_conditions(const vector<Token<ParsedGame::WinConditionsTokenType>>& p_win_conditions_tokens)
{
    m_logger->log(PSLogger::LogType::Log, m_compiler_log_cat, "Compiling win conditions");

    //todo this compiler function does not yet enforce strict tokens ordering

    CompiledGame::WinCondition current_win_condition;

    for(auto& token : p_win_conditions_tokens)
    {
        if(m_has_error)
        {
            break;
        }

        switch (token.token_type)
        {
        case ParsedGame::WinConditionsTokenType::Identifier:
            if(check_identifier_validity(token.str_value,token.token_line,true))
            {
                shared_ptr<CompiledGame::Object> obj = get_obj_by_id(token.str_value).lock();
                if(obj->is_aggregate())
                {
                    detect_error(token,"cannot specify an aggregate object in a win condition");
                }
                else
                {
                    if(current_win_condition.object == nullptr)
                    {
                        current_win_condition.object = obj;
                    }
                    else if( current_win_condition.on_object == nullptr)
                    {
                        current_win_condition.on_object = obj;
                    }
                    else
                    {
                        detect_error(token, "malformed rule");
                    }
                }
            }
            break;
        case ParsedGame::WinConditionsTokenType::Return:
            if(current_win_condition.type != CompiledGame::WinConditionType::None && current_win_condition.object != nullptr)
            {
                m_compiled_game.win_conditions.push_back(current_win_condition);
                current_win_condition = CompiledGame::WinCondition();
            }
            else
            {
                //todo could be an error or just a line jump
            }
            break;
        case ParsedGame::WinConditionsTokenType::NO:
        case ParsedGame::WinConditionsTokenType::ALL:
        case ParsedGame::WinConditionsTokenType::SOME:
            if(current_win_condition.type != CompiledGame::WinConditionType::None)
            {
                detect_error(token, "win condition has already a type");
            }
            else
            {
                switch (token.token_type)
                {
                case ParsedGame::WinConditionsTokenType::NO:
                    current_win_condition.type = CompiledGame::WinConditionType::No;
                    break;
                case ParsedGame::WinConditionsTokenType::ALL:
                    current_win_condition.type = CompiledGame::WinConditionType::All;
                    break;
                case ParsedGame::WinConditionsTokenType::SOME:
                    current_win_condition.type = CompiledGame::WinConditionType::Some;
                    break;

                default:
                    detect_error(token,"should not happen");
                    break;
                }

            }

            break;

        case ParsedGame::WinConditionsTokenType::ON:
            //todo this token is not yet explicitely required by the compiler
            break;
        default:
            detect_error(token,"should not happen");
            break;
        }
    }
    m_logger->log(PSLogger::LogType::Log, m_compiler_log_cat, "Finished Compiling win conditions");
}

void Compiler::compile_levels(const vector<Token<ParsedGame::LevelsTokenType>>& p_levels_tokens)
{
    m_logger->log(PSLogger::LogType::Log, m_compiler_log_cat, "Compiling Levels");

    CompiledGame::Level current_level;

    bool compiling_level_first_line = true;
    int current_tiles_number_on_row = 0;
    int current_height = 0;

    bool waiting_for_message_content_or_return = false;

    m_compiled_game.levels_messages.push_back(vector<string>());

    for(auto& token : p_levels_tokens)
    {
        if(m_has_error)
        {
            break;
        }

        if(waiting_for_message_content_or_return)
        {
            if(token.token_type == ParsedGame::LevelsTokenType::MessageContent)
            {
                m_compiled_game.levels_messages.back().push_back(token.str_value);
            }
            else if(token.token_type == ParsedGame::LevelsTokenType::Return)
            {
                m_compiled_game.levels_messages.back().push_back("");
            }
            else
            {
                assert(false);
            }
            waiting_for_message_content_or_return = false;
        }
        else
        {
            switch (token.token_type)
            {
            case ParsedGame::LevelsTokenType::Return:
                if(current_tiles_number_on_row == 0)
                {
                    if(compiling_level_first_line)
                    {
                        //do nothing, means it just empty lines between levels
                        break;
                    }
                    //finished parsing a level
                    current_level.height = current_height;
                    m_compiled_game.levels.push_back(current_level);

                    current_level = CompiledGame::Level();
                    compiling_level_first_line = true;
                    current_tiles_number_on_row = 0;
                    current_height = 0;

                    m_compiled_game.levels_messages.push_back(vector<string>());
                }
                else
                {
                    if(compiling_level_first_line)
                    {
                        //we just finished parsing the first line of the level
                        compiling_level_first_line = false;
                        current_level.width = current_tiles_number_on_row;
                    }
                    else
                    {
                        if(current_tiles_number_on_row != current_level.width)
                        {
                            detect_error(token, "levels must be rectangular, this line does not match the width of the first one.");
                        }
                    }
                    current_tiles_number_on_row = 0;
                    ++current_height;
                }

                break;

            case ParsedGame::LevelsTokenType::LevelTile:
                if(check_identifier_validity(token.str_value,token.token_line, true))
                {
                    shared_ptr<CompiledGame::Object> obj = get_obj_by_id(token.str_value).lock();

                    if(obj->is_properties())
                    {
                        detect_error(token,"identifier is a property, which cannot be added to a level since it's ambiguous");
                        break;
                    }

                    current_level.cells.push_back(CompiledGame::Cell());
                    obj->GetAllPrimaryObjects(current_level.cells.back().objects);

                    //add the default background obj if there's not yet one specified for the tile
                    bool found_a_background_obj = false;
                    for(weak_ptr<CompiledGame::PrimaryObject> prim_obj : current_level.cells.back().objects)
                    {
                        if(m_background_object.lock()->defines(prim_obj.lock()))
                        {
                            found_a_background_obj = true;
                            break;
                        }
                    }
                    if(!found_a_background_obj)
                    {
                        current_level.cells.back().objects.push_back(m_default_background_object);
                    }

                    ++current_tiles_number_on_row;
                }
                break;

            case ParsedGame::LevelsTokenType::MESSAGE:
                waiting_for_message_content_or_return = true;
                break;

            default:
                assert(false);
                break;
            }
        }
    }

    //Adding the level or the message here in case there was no return token after the last level or last message
    if(compiling_level_first_line == false)
    {
        current_level.height = current_height;
        m_compiled_game.levels.push_back(current_level);
    }
    else if( waiting_for_message_content_or_return)
    {
        m_compiled_game.levels_messages.back().push_back("");
    }

    m_logger->log(PSLogger::LogType::Log, m_compiler_log_cat, "Finished Compiling Levels");
}

void Compiler::reference_collision_layers_in_objects()
{
    for(shared_ptr<CompiledGame::CollisionLayer> col_layer : m_compiled_game.collision_layers)
    {
        for(weak_ptr<CompiledGame::PrimaryObject> obj : col_layer->objects)
        {
            obj.lock()->collision_layer = col_layer;
        }
    }
}

void Compiler::verify_rules_and_compute_deltas(vector<CompiledGame::Rule>& p_rules)
{
    for(int r=0; r < p_rules.size(); ++r)
    {
        CompiledGame::Rule& rule = p_rules[r];

        //verify there is the same number of patterns on both sides of the arrow
        if(rule.match_patterns.size() != rule.result_patterns.size())
        {
            detect_error(rule.rule_line,"There is not the same number of patterns on both sides of the arrow.");
            return;
        }

        //verify each pattern pair have the same number of cells
        for(int p = 0; p < rule.match_patterns.size(); ++p)
        {
            CompiledGame::Pattern match_pattern = rule.match_patterns[p];
            CompiledGame::Pattern result_pattern = rule.result_patterns[p];
            if(match_pattern.cells.size() != result_pattern.cells.size())
            {
                detect_error(rule.rule_line,"There is not the same number of cells in pattern n°"+to_string(p+1)+".");
                return;
            }

            for(int c = 0; c < match_pattern.cells.size(); ++c)
            {
                CompiledGame::CellRule match_cell_rule = match_pattern.cells[c];
                CompiledGame::CellRule result_cell_rule = result_pattern.cells[c];

                //verify validity of ... symbols
                if(match_cell_rule.is_wildcard_cell || result_cell_rule.is_wildcard_cell)
                {
                    if(!match_cell_rule.is_wildcard_cell || !result_cell_rule.is_wildcard_cell)
                    {
                        detect_error(rule.rule_line, "... must be at the same positions on both side of the arrow.");
                        return;
                    }

                    if(match_cell_rule.content.size() > 0  || result_cell_rule.content.size() > 0)
                    {
                        detect_error(rule.rule_line, "When there is ... in a cell there cannot be anything else with it.");
                        return;
                    }

                    if(c == 0 || c == match_pattern.cells.size() - 1)
                    {
                        detect_error(rule.rule_line, "... cannot be in the first or last cell.");
                        return;
                    }

                    if(match_pattern.cells[c+1].is_wildcard_cell)
                    {
                        detect_error(rule.rule_line, "... cannot be followed by another ...");
                        return;
                    }
                }

                //todo should we disallow Aggregates objects ?
            }
        }

        multimap<shared_ptr<CompiledGame::Object>,std::tuple<int,int>> orphan_ambiguous_objects_location; //first int the pattern index, second is the cell index

        //compute deltas
        for(int p = 0; p < rule.match_patterns.size(); ++p)
        {
            CompiledGame::Pattern match_pattern = rule.match_patterns[p];
            CompiledGame::Pattern result_pattern = rule.result_patterns[p];

            for(int c = 0; c < match_pattern.cells.size(); ++c)
            {
                CompiledGame::CellRule match_cell_rule = match_pattern.cells[c];
                CompiledGame::CellRule result_cell_rule = result_pattern.cells[c];

                for(auto match_pair : match_cell_rule.content)
                {
                    const pair<const shared_ptr<CompiledGame::Object>, CompiledGame::EntityRuleInfo>* equivalent_result_pair = nullptr;
                    for(const auto& result_pair : result_cell_rule.content)
                    {
                        if(result_pair.first == match_pair.first)
                        {
                            equivalent_result_pair = &result_pair;
                            break;
                        }
                    }

                    if(equivalent_result_pair == nullptr)
                    {
                        if(match_pair.second != CompiledGame::EntityRuleInfo::No)
                        {
                            rule.deltas.push_back(CompiledGame::Delta(p,c,c,match_pair.first,CompiledGame::ObjectDeltaType::Disappear));
                        }

                        orphan_ambiguous_objects_location.insert(std::make_pair(match_pair.first,std::make_tuple(p,c)));
                    }
                    else if(equivalent_result_pair->second != match_pair.second)
                    {
                        if(equivalent_result_pair->second == CompiledGame::EntityRuleInfo::No)
                        {
                            rule.deltas.push_back(CompiledGame::Delta(p,c,c,match_pair.first,CompiledGame::ObjectDeltaType::Disappear));
                        }
                        else
                        {
                            if(match_pair.second == CompiledGame::EntityRuleInfo::No)
                            {
                                rule.deltas.push_back(CompiledGame::Delta(p,c,c,match_pair.first,CompiledGame::ObjectDeltaType::Appear));
                            }

                            if(equivalent_result_pair->second == CompiledGame::EntityRuleInfo::Moving
                            || equivalent_result_pair->second == CompiledGame::EntityRuleInfo::Parallel
                            || equivalent_result_pair->second == CompiledGame::EntityRuleInfo::Perpendicular
                            || equivalent_result_pair->second == CompiledGame::EntityRuleInfo::Vertical
                            || equivalent_result_pair->second == CompiledGame::EntityRuleInfo::Horizontal
                            || equivalent_result_pair->second == CompiledGame::EntityRuleInfo::Orthogonal)
                            {
                                //this would be an error since its ambiguous,
                                detect_error(rule.rule_line, "ambiguous delta");//todo add more information
                            }
                            else if(equivalent_result_pair->second == CompiledGame::EntityRuleInfo::None
                            || equivalent_result_pair->second == CompiledGame::EntityRuleInfo::Stationary  )
                            {
                                rule.deltas.push_back(CompiledGame::Delta(p,c,c,match_pair.first,CompiledGame::ObjectDeltaType::Stationary));
                            }
                            else
                            {
                                optional<CompiledGame::ObjectDeltaType> delta_type = str_to_enum(enum_to_str(equivalent_result_pair->second,CompiledGame::to_entity_rule_info).value_or("ERROR"), CompiledGame::to_object_delta_type);
                                assert(delta_type.has_value());
                                rule.deltas.push_back(CompiledGame::Delta(p,c,c,match_pair.first,delta_type.value()));
                            }
                        }
                    }
                }


                for(auto result_pair : result_cell_rule.content)
                {
                    const pair<const shared_ptr<CompiledGame::Object>, CompiledGame::EntityRuleInfo>* equivalent_match_pair = nullptr;
                    for(const auto& match_pair : match_cell_rule.content)
                    {
                        if(result_pair.first == match_pair.first)
                        {
                            equivalent_match_pair = &result_pair;
                            break;
                        }
                    }

                    if(equivalent_match_pair != nullptr)
                    {
                        //was already handled in the for loop around match_cell_rule_content
                        continue;
                    }
                    else
                    {
                        int match_pattern_location = p;
                        int match_cell_location = c;
                        if(result_pair.first->is_properties())
                        {
                            //try to find if there is an unmatched object in another cell in the match patterns
                            if(orphan_ambiguous_objects_location.count(result_pair.first) != 1)
                            {
                                detect_error(rule.rule_line,"could not resolve ambiguity with "+result_pair.first->identifier+"."); //todo add more precise informations
                            }
                            else
                            {
                                std::tie(match_pattern_location,match_cell_location) = orphan_ambiguous_objects_location.find(result_pair.first)->second;
                            }
                        }

                        if(result_pair.second == CompiledGame::EntityRuleInfo::No)
                        {
                            CompiledGame::Delta delta(match_pattern_location,match_cell_location,c,result_pair.first,CompiledGame::ObjectDeltaType::Disappear);
                            delta.is_optional = true;
                            rule.deltas.push_back(delta);
                        }
                        else
                        {
                            rule.deltas.push_back(CompiledGame::Delta(match_pattern_location,match_cell_location,c,result_pair.first,CompiledGame::ObjectDeltaType::Appear));

                            if(result_pair.second == CompiledGame::EntityRuleInfo::None
                            || result_pair.second == CompiledGame::EntityRuleInfo::Stationary )
                            {
                                //we  consider None and Stationary to be handled by the appear delta
                            }
                            else if(result_pair.second == CompiledGame::EntityRuleInfo::Moving
                            || result_pair.second == CompiledGame::EntityRuleInfo::Parallel
                            || result_pair.second == CompiledGame::EntityRuleInfo::Perpendicular
                            || result_pair.second == CompiledGame::EntityRuleInfo::Vertical
                            || result_pair.second == CompiledGame::EntityRuleInfo::Horizontal
                            || result_pair.second == CompiledGame::EntityRuleInfo::Orthogonal)
                            {
                                //this would be an error since its ambiguous, a pass in the rule compilation should have detected that
                                detect_error(rule.rule_line,"ambiguous delta"); //todo add more information
                            }
                            else
                            {
                                optional<CompiledGame::ObjectDeltaType> delta_type = str_to_enum(enum_to_str(result_pair.second,CompiledGame::to_entity_rule_info).value_or("ERROR"), CompiledGame::to_object_delta_type);
                                assert(delta_type.has_value());
                                rule.deltas.push_back(CompiledGame::Delta(match_pattern_location,match_cell_location,c,result_pair.first,delta_type.value()));
                            }
                        }
                    }
                }
            }
        }
    }
}

weak_ptr<CompiledGame::Object> Compiler::get_obj_by_id(const string& p_id)
{
    ci_equal comp_equal;
    auto found_existing_object = std::find_if(m_compiled_game.objects.begin(),m_compiled_game.objects.end(),[&](shared_ptr<CompiledGame::Object> obj){
        return comp_equal(obj->identifier, p_id);
    });
    if(found_existing_object != m_compiled_game.objects.end() )
    {
        return *found_existing_object;
    }
    return std::weak_ptr<CompiledGame::Object>();
}

bool Compiler::check_identifier_validity(const string& p_id, int p_identifier_line_numbler, bool p_should_already_exist)
{
    if(p_id.empty())
    {
        detect_error(p_identifier_line_numbler,"Literal token has an empty an empty str_value. This should never happen and suggest an error in the parser implementation.");
        return false;
    }

    //find if objects was already declared
    ci_equal comp_equal;
    auto found_existing_object = std::find_if(m_compiled_game.objects.begin(),m_compiled_game.objects.end(),[&](shared_ptr<CompiledGame::Object> obj){
        return comp_equal(obj->identifier, p_id);
    });
    if(found_existing_object != m_compiled_game.objects.end() && !p_should_already_exist)
    {
        detect_error(p_identifier_line_numbler,"Another object is already called like that !");
        return false;
    }
    else if(found_existing_object == m_compiled_game.objects.end() && p_should_already_exist)
    {
        detect_error(p_identifier_line_numbler,"trying to reference Object \"" + p_id + "\" but it does not exists !");
        return false;
    }

    return true;
}

template<typename T>
void Compiler::detect_error(Token<T> p_token, string p_error_msg, bool p_is_warning /*= false*/)
{
    if(!p_is_warning)
    {
        m_has_error = true;
    }
    m_logger->log(p_is_warning ? PSLogger::LogType::Warning : PSLogger::LogType::Error, m_compiler_log_cat, "(l." + to_string(p_token.token_line) + ") : " + p_error_msg);
}

void Compiler::detect_error(int p_line_number, string p_error_msg, bool p_is_warning /*= false*/)
{
    if(!p_is_warning)
    {
        m_has_error = true;
    }
    m_logger->log(p_is_warning ? PSLogger::LogType::Warning : PSLogger::LogType::Error, m_compiler_log_cat, "(l." + to_string(p_line_number) + ") : " + p_error_msg);
}
