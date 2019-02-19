#include <bitset>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

using namespace std;

enum class attr : unsigned {
  VOID, INT, NULLX, STRING, STRUCT, ARRAY, FUNCTION, VARIABLE, FIELD,
  TYPEID, PARAM, LOCAL, LVAL, CONST, VREG, VADDR, BITSET_SIZE,
};
using attr_bitset = bitset<unsigned(attr::BITSET_SIZE)>;

const string to_string(attr attribute) {
  static const unordered_map<attr, string> hash{
      {attr::VOID, "void"},
      {attr::INT, "int"},
      {attr::NULLX, "null"},
      {attr::STRING, "string"},
      {attr::STRUCT, "struct"},
      {attr::ARRAY, "array"},
      {attr::FUNCTION, "function"},
      {attr::VARIABLE, "variable"},
      {attr::FIELD, "field"},
      {attr::TYPEID, "typeid"},
      {attr::PARAM, "param"},
      {attr::LOCAL, "local"},
      {attr::LVAL, "lval"},
      {attr::CONST, "const"},
      {attr::VREG, "vreg"},
      {attr::VADDR, "vaddr"},
      {attr::BITSET_SIZE, "bitset_size"},
  };
  auto str = hash.find(attribute);
  if (str == hash.end()) {
    throw invalid_argument(string(__PRETTY_FUNCTION__) + ": "
                               + to_string(unsigned(attribute)));
  }
  return str->second;
}

int main() {
  static vector<attr> attrs{
      attr::VOID, attr::INT, attr::NULLX, attr::STRING, attr::STRUCT,
      attr::ARRAY, attr::FUNCTION, attr::VARIABLE, attr::FIELD,
      attr::TYPEID, attr::PARAM, attr::LOCAL, attr::LVAL, attr::CONST,
      attr::VREG, attr::VADDR, attr::BITSET_SIZE,
  };
  for (const auto attrib: attrs) {
    cout << unsigned(attrib) << " = " << to_string(attrib) << endl;
  }
  try {
    cout << to_string(static_cast<attr> (1024)) << endl;
  } catch (invalid_argument &what) {
    cout << "invalid_argument: " << what.what() << endl;
  }
}
