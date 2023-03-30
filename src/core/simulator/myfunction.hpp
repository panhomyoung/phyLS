#ifndef MYFUNCTION_HPP
#define MYFUNCTION_HPP

#include <iostream>
#include <string>
#include <vector>

namespace phyLS {
static std::vector<std::string> m_split(const std::string& input,
                                        const std::string& pred) {
  std::vector<std::string> result;
  std::string temp = "";
  unsigned count1 = input.size();
  unsigned count2 = pred.size();
  unsigned j;
  for (size_t i = 0; i < count1; i++) {
    for (j = 0; j < count2; j++) {
      if (input[i] == pred[j]) {
        break;
      }
    }
    // if input[i] != pred中的任何一个 该字符加到temp上
    if (j == count2)
      temp += input[i];
    else {
      if (!temp.empty()) {
        result.push_back(temp);
        temp.clear();
      }
    }
  }
  return result;
}

static void seg_fault(const std::string& name, int size, int idx) {
  std::cout << name << "  " << size << " : " << idx << std::endl;
}
}  // namespace phyLS

#endif