#ifndef RIEL_H_
#define RIEL_H_

#include <regex>
#include <vector>

// = = = =
// Macros
// = = = =

#ifndef RIEL_DISALLOW_DUPLICATION
#define RIEL_DISALLOW_DUPLICATION(Kind)                                        \
  Kind(const Kind &) = delete;                                                 \
  void operator=(const Kind &) = delete
#endif

#ifndef RIEL_DISALLOW_MOVING
#define RIEL_DISALLOW_MOVING(Kind)                                             \
  Kind(Kind &&) = delete;                                                      \
  void operator=(Kind &&) = delete
#endif

#ifndef RIEL_DISALLOW_ALL
#define RIEL_DISALLOW_ALL(Kind)                                                \
  RIEL_DISALLOW_DUPLICATION(Kind);                                             \
  RIEL_DISALLOW_MOVING(Kind)
#endif

#ifndef RIEL_EXPORT
#define RIEL_EXPORT __attribute__((visibility("default")))
#endif

#ifndef RIEL_INTERNAL
#define RIEL_INTERNAL __attribute__((visibility("hidden")))
#endif

// = = =
// root
// = = =

namespace riel {

// = = =
// Node
// = = =

class RIEL_EXPORT Children {
public:
  virtual ~Children();

  virtual const std::unique_ptr<class Node> &
  operator[](std::size_t child_index) const noexcept = 0;

  virtual void append(std::unique_ptr<Node> &&child) = 0;

  virtual std::size_t size() const noexcept = 0;

protected:
  inline Children() = default;

private:
  RIEL_DISALLOW_ALL(Children);
};

struct Type {
  enum type {
    AGGREGATE,
    UNION,
    PROJECT,
    SCAN,
  };
};

class RIEL_EXPORT Node {
public:
  virtual ~Node();

  inline virtual Children &children() const noexcept = 0;

  virtual Type::type id() const noexcept = 0;

  virtual void Accept(const class Visitor &) const = 0;

protected:
  inline Node() = default;

private:
  RIEL_DISALLOW_ALL(Node);
};

// = = = = = = = = = =
// Visitation policies
// = = = = = = = = = =

class RIEL_EXPORT Visitable {
public:
  virtual inline ~Visitable() = default;

  virtual void Accept(const class Visitor &) const = 0;

private:
  RIEL_DISALLOW_ALL(Visitable);
};

// = = = = = = =
// Computations
// = = = = = = =

class RIEL_EXPORT Computable {
public:
  virtual inline ~Computable() = default;

  virtual void compute() const = 0;

private:
  RIEL_DISALLOW_ALL(Computable);
};

// = = = = = =
// Persistance
// = = = = = =

namespace io {

class RIEL_EXPORT Buffer {
public:
  virtual inline ~Buffer() = default;

private:
  RIEL_DISALLOW_ALL(Buffer);
};

class RIEL_EXPORT DomainObject {};
class RIEL_EXPORT Criteria {};

class RIEL_EXPORT Repository {
public:
  class RIEL_EXPORT Strategy {
  public:
    inline virtual ~Strategy() = default;

    virtual std::vector<DomainObject> matching(const Criteria &) const
        noexcept = 0;

  private:
    RIEL_DISALLOW_ALL(Strategy);
  };

  virtual inline ~Repository() = default;

private:
  const std::unique_ptr<Strategy> strategy;

  RIEL_DISALLOW_ALL(Repository);
};

}  // namespace io

// = = = = = = = = = = =
// Output reporesentable
// = = = = = = = = = = =

class RIEL_EXPORT Representable {
public:
  virtual ~Representable();
  virtual explicit operator std::string() const noexcept = 0;

protected:
  inline Representable() = default;

private:
  RIEL_DISALLOW_ALL(Representable);
};

template <template <class = std::unique_ptr<Node>, class...> class Container>
class RIEL_EXPORT ContiguousIterator : public Children {
public:
  using const_iterator = typename Container<>::const_iterator;

  virtual const_iterator begin() const noexcept = 0;
  virtual const_iterator end() const noexcept   = 0;
};

class ContiguousChildren : public ContiguousIterator<std::vector> {
public:
  inline ContiguousChildren() = default;
  ~ContiguousChildren() final;

  const std::unique_ptr<class Node> &operator[](const std::size_t index) const
      noexcept final {
    return nodes_[index];
  }

  void append(std::unique_ptr<Node> &&child) final {
    nodes_.push_back(std::move(child));
  }

  std::size_t size() const noexcept final { return nodes_.size(); }

  const_iterator begin() const noexcept final { return nodes_.begin(); }
  const_iterator end() const noexcept final { return nodes_.end(); }

private:
  std::vector<std::unique_ptr<Node>> nodes_;

  RIEL_DISALLOW_ALL(ContiguousChildren);
};

class ContiguousNode : public Node {
public:
  ~ContiguousNode() override;

  inline Children &children() const noexcept final { return *children_; }

protected:
  explicit ContiguousNode()
      : children_{std::make_unique<ContiguousChildren>()} {}

private:
  std::unique_ptr<ContiguousChildren> children_;

  RIEL_DISALLOW_ALL(ContiguousNode);
};

class RIEL_INTERNAL RepresentableNode : public ContiguousNode,
                                        public Representable {
  struct RIEL_INTERNAL OStreamNode {
    const RepresentableNode *actual;
    const std::size_t        indent;
  };

  friend inline std::ostream &operator<<(std::ostream &     ostream,
                                         const OStreamNode &ostream_node) {
    ostream << std::string{ostream_node.indent,
                           ' ',
                           std::string::allocator_type()}
            << static_cast<std::string>(*ostream_node.actual);
    for (const auto &node : dynamic_cast<const ContiguousChildren &>(
             ostream_node.actual->children())) {
      ostream << std::endl
              << OStreamNode{dynamic_cast<RepresentableNode *>(node.get()),
                             ostream_node.indent + 2};
    }
    return ostream;
  }

  friend inline std::ostream &
  operator<<(std::ostream &                            ostream,
             const std::unique_ptr<RepresentableNode> &node) {
    return ostream << OStreamNode{node.get(), 0};
  }

public:
  ~RepresentableNode() override;

protected:
  inline RepresentableNode() = default;

  template <class Container, class... Args>
  static inline void
  inserts(std::ostream &stream, const Container &container, Args... args) {
    const auto &end = std::cend(container);
    auto        it  = std::cbegin(container);

    _inserts(stream, it, args...);
    while (end != ++it) {
      stream << ", ";
      _inserts(stream, it, args...);
    }
  }

  template <class Container>
  static inline void inserts(std::ostream &stream, const Container &container) {
    inserts(stream, container, [](const auto &it) { return it; });
  }

private:
  template <class ForwardIterator, class Callable>
  static inline void
  _inserts(std::ostream &stream, ForwardIterator &it, Callable &&callback) {
    stream << callback(*it);
  }

  template <class ForwardIterator>
  static inline void _inserts(std::ostream &stream,
                              ForwardIterator & /*unused*/,
                              const char *value) {
    stream << value;
  }

  template <class ForwardIterator, class T, class... Args>
  static inline void
  _inserts(std::ostream &stream, ForwardIterator &it, T &&value, Args... args) {
    _inserts(stream, it, std::forward<std::decay_t<T>>(value));
    _inserts(stream, it, args...);
  }

  RIEL_DISALLOW_ALL(RepresentableNode);
};

// = = =
// Nodes
// = = =

// TODO(gcca): here ContiguousVectorChildren

class RIEL_EXPORT ScanNode : public RepresentableNode {
public:
  explicit ScanNode(const std::vector<std::string> &&path) : path_{path} {}

  ~ScanNode() final;

  const std::vector<std::string> &path() const { return path_; }

  Type::type id() const noexcept final { return Type::SCAN; }

  void Accept(const Visitor & /*visitor*/) const final;

  explicit operator std::string() const noexcept final {
    std::ostringstream stream{std::ios_base::out};
    stream << "Scan(table=[[";
    inserts(stream, path_);
    stream << "]])";

    return stream.str();
  }

private:
  const std::vector<std::string> path_;

  RIEL_DISALLOW_ALL(ScanNode);
};

class RIEL_EXPORT UnionNode : public RepresentableNode {
public:
  explicit UnionNode(const bool all) : all_{all} {}

  ~UnionNode() final;

  bool all() const { return all_; }

  Type::type id() const noexcept final { return Type::UNION; }

  void Accept(const Visitor & /*visitor*/) const final;

  explicit operator std::string() const noexcept final {
    std::ostringstream stream{std::ios_base::out};
    stream << "Union(all=[" << std::boolalpha << all_ << "])";
    return stream.str();
  }

private:
  const bool all_ : __SYSCALL_WORDSIZE;

  RIEL_DISALLOW_ALL(UnionNode);
};

class RIEL_EXPORT AggregateNode : public RepresentableNode {
public:
  explicit AggregateNode(std::vector<std::size_t> &&group_indices)
      : group_indices_{std::move(group_indices)} {}

  ~AggregateNode() final;

  const std::vector<std::size_t> &group_indices() const {
    return group_indices_;
  }

  Type::type id() const noexcept final { return Type::AGGREGATE; }

  void Accept(const Visitor & /*visitor*/) const final;

  explicit operator std::string() const noexcept final {
    std::ostringstream stream{std::ios_base::out};
    stream << "Aggregate(group=[{";
    inserts(stream, group_indices_);
    stream << "}])";

    return stream.str();
  }

private:
  std::vector<std::size_t> group_indices_;

  RIEL_DISALLOW_ALL(AggregateNode);
};

class RIEL_EXPORT ProjectNode : public RepresentableNode {
public:
  explicit ProjectNode(
      const std::vector<std::pair<std::string, std::size_t>> &&pairs)
      : pairs_{pairs} {}

  ~ProjectNode() final;

  const std::vector<std::pair<std::string, std::size_t>> &pairs() const {
    return pairs_;
  }

  Type::type id() const noexcept final { return Type::PROJECT; }

  void Accept(const Visitor & /*visitor*/) const final;

  explicit operator std::string() const noexcept final {
    std::ostringstream stream{std::ios_base::out};
    stream << "Project(";
    inserts(
        stream,
        pairs_,
        [](const auto &pair) { return pair.first; },
        "=[$",
        [](const auto &pair) { return std::to_string(pair.second); },
        "]");
    stream << ")";

    return stream.str();
  }

private:
  std::vector<std::pair<std::string, std::size_t>> pairs_;

  RIEL_DISALLOW_ALL(ProjectNode);
};

class RIEL_EXPORT Visitor {
public:
  virtual ~Visitor();

#define VISIT_NODE(Node)                                                       \
  virtual void Visit(const Node &) const {                                     \
    throw std::runtime_error("Not implemented for " #Node);                    \
  }

  VISIT_NODE(ScanNode)
  VISIT_NODE(UnionNode)
  VISIT_NODE(AggregateNode)
  VISIT_NODE(ProjectNode)

#undef VISIT
private:
  RIEL_DISALLOW_ALL(Visitor);
};

namespace building {

// = = = =
// Builder
// = = = =

class Builder {
public:
  virtual ~Builder();

  virtual std::unique_ptr<Node> Build() = 0;

protected:
  inline Builder() = default;

private:
  RIEL_DISALLOW_ALL(Builder);
};


class PropertiesBuilder : public Builder {
public:
  using Property   = std::pair<std::string, std::string>;
  using Properties = std::vector<Property>;

  explicit PropertiesBuilder(const Properties &&properties)
      : properties_{properties} {}

  ~PropertiesBuilder() override;

protected:
  class ctype : public std::ctype<char> {
  public:
    virtual mask const *table();
    ctype() : std::ctype<char>(table(), false, 0), unused_{0} {}
    const std::int64_t unused_ : __SYSCALL_WORDSIZE - _GLIBCXX_NUM_CXX11_FACETS;
  };

  inline const Properties &properties() const { return properties_; }

private:
  Properties properties_;

  RIEL_DISALLOW_ALL(PropertiesBuilder);
};

class UnionPropertiesBuilder : public PropertiesBuilder {
public:
  using PropertiesBuilder::PropertiesBuilder;

  ~UnionPropertiesBuilder() final;

  std::unique_ptr<Node> Build() final {
    if (1 != properties().size()) {
      throw std::runtime_error("Bad union properties with size = " +
                               std::to_string(properties().size()));
    }

    const Property &property = properties()[0];

    if ("all" != property.first) {
      throw std::runtime_error("Union property builder with property " +
                               property.first);
    }

    if ("true" == property.second) { return std::make_unique<UnionNode>(true); }
    if ("false" == property.second) {
      return std::make_unique<UnionNode>(false);
    }
    throw std::runtime_error("Union property builder with value " +
                             property.second);
  }

private:
  RIEL_DISALLOW_ALL(UnionPropertiesBuilder);
};

class ScanPropertiesBuilder : public PropertiesBuilder {
public:
  using PropertiesBuilder::PropertiesBuilder;

  ~ScanPropertiesBuilder() final;

  std::unique_ptr<Node> Build() final {
    if (1 != properties().size()) {
      throw std::runtime_error("Bad scan properties builder size = " +
                               std::to_string(properties().size()));
    }

    const Property &property = properties()[0];

    if ("table" != property.first) {
      throw std::runtime_error("Scan property builder with property " +
                               property.first);
    }

    std::istringstream stream{property.second, std::ios_base::in};
    stream.imbue(std::locale(std::locale(), new ctype));
    stream.seekg(1);

    std::vector<std::string> parts;
    std::string              part;
    while (stream >> part) { parts.emplace_back(std::move(part)); }

    return std::make_unique<ScanNode>(std::move(parts));
  }

private:
  RIEL_DISALLOW_ALL(ScanPropertiesBuilder);
};

class AggregatePropertiesBuilder : public PropertiesBuilder {
public:
  using PropertiesBuilder::PropertiesBuilder;

  ~AggregatePropertiesBuilder() final;

  std::unique_ptr<Node> Build() final {
    if (1 != properties().size()) {
      throw std::runtime_error("Bad union properties with size = " +
                               std::to_string(properties().size()));
    }

    const Property &property = properties()[0];

    if ("group" != property.first) {
      throw std::runtime_error("Aggregate property builder with property " +
                               property.first);
    }

    std::istringstream stream{property.second, std::ios_base::in};
    stream.imbue(std::locale(std::locale(), new ctype));
    stream.seekg(1);

    const int                base = 10;
    std::vector<std::size_t> parts;
    std::string              part;
    while (stream >> part) {
      parts.emplace_back(std::stoul(part, nullptr, base));
    }

    return std::make_unique<AggregateNode>(std::move(parts));
  }

private:
  RIEL_DISALLOW_ALL(AggregatePropertiesBuilder);
};

class ProjectPropertiesBuilder : public PropertiesBuilder {
public:
  using PropertiesBuilder::PropertiesBuilder;

  ~ProjectPropertiesBuilder() final;

  std::unique_ptr<Node> Build() final {
    if (properties().empty()) {
      throw std::runtime_error("Bad project properties builder size = " +
                               std::to_string(properties().size()));
    }

    const std::size_t base = 10;

    std::vector<std::pair<std::string, std::size_t>> pairs;
    for (const auto &property : properties()) {
      pairs.emplace_back(std::pair<std::string, std::size_t>(
          property.first,
          std::stoul(
              property.second.substr(1, std::string::npos), nullptr, base)));
    }

    return std::make_unique<ProjectNode>(std::move(pairs));
  }

private:
  RIEL_DISALLOW_ALL(ProjectPropertiesBuilder);
};

}  // namespace building

// = = = =
// Parser
// = = = =

/**
 * Parser base class
 */
class RIEL_INTERNAL Parser {
public:
  inline Parser() = default;
  virtual ~Parser();

  virtual std::unique_ptr<Node> parse() = 0;

private:
  RIEL_DISALLOW_ALL(Parser);
};

/**
 * StreamParser
 **/
class RIEL_EXPORT StreamParser : public Parser {
public:
  explicit StreamParser(std::istream &istream) noexcept : istream_{istream} {}

  ~StreamParser() final;

  std::unique_ptr<Node> parse() final {
    std::getline(istream_, format);
    auto root = MakeNode(format);
    std::getline(istream_, format);
    level = 1;
    Traverse(root);
    return root;
  }

private:
  void Traverse(const std::unique_ptr<Node> &parent) {
    while (!format.empty()) {
      if (format[level] != ' ') {
        level -= 2;
        break;
      }
      auto node = MakeNode(format);
      std::getline(istream_, format);
      level += 2;
      Traverse(node);
      parent->children().append(std::move(node));
    }
  }

  using Arguments = std::vector<std::pair<std::string, std::string>>;

  static std::unique_ptr<Node> MakeNode(const std::string &format) {
    std::regex re{
        "^\\s*"                // indent
        "(\\w+)"               // node name
        "\\("                  // start arguments
        "([[:alpha:]]+)=\\[("  // start first
        "[[:lower:]]+"
        "|\\$\\d+"
        "|\\{\\d+(?:,\\s*\\d+)*\\}"
        "|\\[[[:upper:]]+(?:,\\s*[[:upper:]]+)*\\]"
        ")\\]"  // end first
        "(?:,\\s*"
        "(?:([[:alpha:]]+)=\\[("
        "[[:lower:]]+"
        "|\\$\\d+"
        "|\\{\\d+(?:,\\s*\\d+)*\\}"
        "|\\[[[:upper:]]+(?:,\\s*[[:upper:]]+)*\\]"
        ")\\])"
        ")*"
        "\\)$",
        std::regex::ECMAScript};
    std::smatch match{std::smatch::allocator_type()};

    if (std::regex_match(
            format, match, re, std::regex_constants::match_default)) {
      const std::string name = match.str(1);

      Arguments arguments;

      for (std::size_t i = 2; i < match.size(); i += 2) {
        if (match[i].length() == 0) { break; }
        arguments.emplace_back(std::make_pair(match.str(i), match.str(i + 1)));
      }

      const auto builder = MakePropertiesBuilder(name, std::move(arguments));

      return builder->Build();
    }

    throw std::runtime_error("Bad format: '" + format + "'");
  }

  static std::unique_ptr<building::PropertiesBuilder>
  MakePropertiesBuilder(const std::string &name, Arguments &&args) {
#define MAKE_BUILDER(Name)                                                     \
  if (name == #Name) {                                                         \
    return std::make_unique<building::Name##PropertiesBuilder>(                \
        std::move(args));                                                      \
  }

    MAKE_BUILDER(Aggregate);
    MAKE_BUILDER(Union);
    MAKE_BUILDER(Project);
    MAKE_BUILDER(Scan);

    throw std::runtime_error("Unreachable MakePropertiesBuilder");

#undef MAKE_BUILDER
  }

  std::istream &istream_;
  std::string   format;
  std::size_t   level{};

  RIEL_DISALLOW_ALL(StreamParser);
};

}  // namespace riel

#endif
