#include "riel.h"

namespace riel {

Children::~Children()                     = default;
ContiguousChildren::~ContiguousChildren() = default;

Node::~Node() = default;

ContiguousNode::~ContiguousNode() = default;

Representable::~Representable() = default;

ScanNode::~ScanNode()           = default;
UnionNode::~UnionNode()         = default;
AggregateNode::~AggregateNode() = default;
ProjectNode::~ProjectNode()     = default;

RepresentableNode::~RepresentableNode() = default;

namespace building {

Builder::~Builder() = default;

PropertiesBuilder::~PropertiesBuilder() = default;

UnionPropertiesBuilder::~UnionPropertiesBuilder()         = default;
ScanPropertiesBuilder::~ScanPropertiesBuilder()           = default;
AggregatePropertiesBuilder::~AggregatePropertiesBuilder() = default;
ProjectPropertiesBuilder::~ProjectPropertiesBuilder()     = default;

PropertiesBuilder::ctype::mask const *PropertiesBuilder::ctype::table() {
  using iterator_type =
      std::vector<std::ctype_base::mask>::const_iterator::iterator_type;
  const iterator_type begin = classic_table();
  const iterator_type end   = begin + static_cast<std::ptrdiff_t>(table_size);

  static auto &table = *new std::vector<std::ctype<char>::mask>(
      begin, end, std::allocator<std::ctype<char>::mask>());
  table[','] = table[']'] = table['}'] = static_cast<mask>(space);
  return &table[0];
}

}  // namespace building

Parser::~Parser() = default;

StreamParser::~StreamParser() = default;

Visitor::~Visitor() = default;

#define ACCEPT_VISITOR(Node)                                                   \
  void Node::Accept(const Visitor &visitor) const { visitor.Visit(*this); }

ACCEPT_VISITOR(ScanNode);
ACCEPT_VISITOR(UnionNode);
ACCEPT_VISITOR(AggregateNode);
ACCEPT_VISITOR(ProjectNode);

#undef ACCEPT_VISITOR


}  // namespace riel
