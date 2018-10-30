#include <riel/riel.h>

#include <gtest/gtest.h>

class FormattedNodeTest : public ::testing::Test {
protected:
  ~FormattedNodeTest() noexcept;
};

FormattedNodeTest::~FormattedNodeTest() noexcept = default;

TEST_F(FormattedNodeTest, OutputTree) {
  std::unique_ptr<riel::RepresentableNode> root =
      std::make_unique<riel::AggregateNode>(std::vector<std::size_t>{0, 1});

  auto _union = std::make_unique<riel::UnionNode>(true);

  auto proj1 = std::make_unique<riel::ProjectNode>(
      std::vector<std::pair<std::string, std::size_t>>{
          {"SECTOR", 0},
          {"NAME", 1},
      });
  proj1->children().append(std::make_unique<riel::ScanNode>(
      std::vector<std::string>{"CATALOG", "SALES", "NATIONAL"}));

  auto proj2 = std::make_unique<riel::ProjectNode>(
      std::vector<std::pair<std::string, std::size_t>>{
          {"SECTOR", 0},
          {"NAME", 1},
      });
  proj2->children().append(std::make_unique<riel::ScanNode>(
      std::vector<std::string>{"CATALOG", "SALES", "INTERNATIONAL"}));

  _union->children().append(std::move(proj1));
  _union->children().append(std::move(proj2));
  root->children().append(std::move(_union));

  std::ostringstream ostream;
  ostream << root;

  EXPECT_EQ(("Aggregate(group=[{0, 1}])\n"
             "  Union(all=[true])\n"
             "    Project(SECTOR=[$0], NAME=[$1])\n"
             "      Scan(table=[[CATALOG, SALES, NATIONAL]])\n"
             "    Project(SECTOR=[$0], NAME=[$1])\n"
             "      Scan(table=[[CATALOG, SALES, INTERNATIONAL]])"),
            ostream.str());
}

class StreamParserTest : public ::testing::Test {
protected:
  ~StreamParserTest() noexcept;
};

StreamParserTest::~StreamParserTest() noexcept = default;

TEST_F(StreamParserTest, ParseFourLevelsWithASibling) {
  std::istringstream stream{
      "Aggregate(group=[{0, 1}])\n"
      "  Union(all=[true])\n"
      "    Project(SECTOR=[$0], NAME=[$1])\n"
      "      Scan(table=[[RECORDS, SALES, NATIONAL]])\n"
      "    Project(SECTOR=[$0], NAME=[$1])\n"
      "      Scan(table=[[RECORDS, SALES, INTERNATIONAL]])\n"};
  riel::StreamParser parser = riel::StreamParser{stream};

  const auto root = parser.parse();

  EXPECT_EQ(1, root->children().size());
  EXPECT_EQ(2, root->children()[0]->children().size());
  EXPECT_EQ(1, root->children()[0]->children()[0]->children().size());
  EXPECT_EQ(
      static_cast<std::size_t>(0),
      root->children()[0]->children()[0]->children()[0]->children().size());
  EXPECT_EQ(1, root->children()[0]->children()[1]->children().size());
  EXPECT_EQ(
      static_cast<std::size_t>(0),
      root->children()[0]->children()[1]->children()[0]->children().size());

  EXPECT_EQ(riel::Type::AGGREGATE, root->id());
  EXPECT_EQ(riel::Type::UNION, root->children()[0]->id());
  EXPECT_EQ(riel::Type::PROJECT, root->children()[0]->children()[0]->id());
  EXPECT_EQ(riel::Type::SCAN,
            root->children()[0]->children()[0]->children()[0]->id());
  EXPECT_EQ(riel::Type::PROJECT, root->children()[0]->children()[1]->id());
  EXPECT_EQ(riel::Type::SCAN,
            root->children()[0]->children()[1]->children()[0]->id());

  const riel::AggregateNode *aggregate =
      dynamic_cast<riel::AggregateNode *>(root.get());
  EXPECT_EQ((std::vector<std::size_t>{0, 1}), aggregate->group_indices());

  const auto *_union =
      dynamic_cast<riel::UnionNode *>(aggregate->children()[0].get());
  EXPECT_TRUE(_union->all());

  const auto &projects = _union->children();

  const auto *project1 = dynamic_cast<riel::ProjectNode *>(projects[0].get());
  EXPECT_EQ((std::vector<std::pair<std::string, std::size_t>>{
                {"SECTOR", 0},
                {"NAME", 1},
            }),
            project1->pairs());
  const auto *scan1 =
      dynamic_cast<riel::ScanNode *>(project1->children()[0].get());
  EXPECT_EQ((std::vector<std::string>{"RECORDS", "SALES", "NATIONAL"}),
            scan1->path());

  const auto *project2 = dynamic_cast<riel::ProjectNode *>(projects[1].get());
  EXPECT_EQ((std::vector<std::pair<std::string, std::size_t>>{
                {"SECTOR", 0},
                {"NAME", 1},
            }),
            project2->pairs());
  const auto *scan2 =
      dynamic_cast<riel::ScanNode *>(project2->children()[0].get());
  EXPECT_EQ((std::vector<std::string>{"RECORDS", "SALES", "INTERNATIONAL"}),
            scan2->path());
}
