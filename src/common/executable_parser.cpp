#include <arion/common/executable_parser.hpp>

std::shared_ptr<ARION_EXECUTABLE_PARSER_ATTRIBUTES> ExecutableParser::get_attrs()
{
    return this->attrs;
}

std::vector<std::shared_ptr<struct arion::SEGMENT>> ExecutableParser::get_segments()
{
    return this->segments;
}
