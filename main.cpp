
#include "cnp.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <cassert>
int main() {

  std::string html = R"(
      <html>
        <body>
          <a href="x" target="_blank">A</a>
          <a href="y" target="_self">B</a>
          <div target="_blank">C</div>
          <img src="anything-at-all" target="_blank"/>
        </body>
      </html>

  )";
  auto res = cnp::find_elements_by_attr_val(html, "target", "_blank");
  assert(res.size() == 3);
  assert(res[0].find("target=\"_blank\"") != std::string::npos);
  assert(res[1].find("target=\"_blank\"") != std::string::npos);
  assert(res[2].find("target=\"_blank\"") != std::string::npos);

  std::cout<<"TEST PASSED: find_elements_by_attr_val\n";
  
  std::string url = "https://lichess.org/";

  cnp::init();

  std::string result_text = cnp::download_page(url);
  std::string plain_text = cnp::html_to_text(result_text);

  std::cout << cnp::get_webpage_text(url) << std::endl;

  std::vector<std::string> result;
  result = cnp::get_tags_to_array(result_text, "a");

  std::cout << result.size() << std::endl;
  for (auto s : result) {
    std::cout << s << std::endl;
  }

  std::vector<std::string> elements =
      cnp::find_elements_by_class(result_text, "site-name");

  for (auto s : elements) {
    std::cout << s << std::endl;
  }

  std::vector<std::string> attr_test =
      cnp::find_elements_by_attr_val(result_text, "target", "_blank");
  for (auto s : attr_test) {
    std::cout << s << std::endl;
  }
  std::vector<std::string> urls_in_webpage =
            cnp::extractUrls(result_text);
    for (auto s : urls_in_webpage) {
      std::cout << s << std::endl;
    }

  cnp::cleanup();

  
  return 0;
}
