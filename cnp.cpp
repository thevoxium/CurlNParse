#include "cnp.h"
#include <algorithm>
#include <cctype>
#include <curl/curl.h>
#include <iostream>
#include <regex>
#include <vector>

namespace cnp {

size_t writeCallback(void *content, size_t size, size_t nmemb, void *userdata) {
  std::string *str = static_cast<std::string *>(userdata);
  str->append(static_cast<char *>(content), size * nmemb);
  return size * nmemb;
}

bool init() { return curl_global_init(CURL_GLOBAL_ALL) == CURLE_OK; }

void cleanup() { curl_global_cleanup(); }

std::string download_page(const std::string &url) {
  std::string content;
  long http_code = 0;
  CURL *curl = curl_easy_init();
  bool result = false;

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);
    curl_easy_setopt(
        curl, CURLOPT_USERAGENT,
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
      if (http_code == 200) {
        result = true;
      } else {
        std::cerr << "HTTP Error: " << http_code << std::endl;
      }
    } else {
      std::cerr << "Curl Error: " << curl_easy_strerror(res) << std::endl;
    }
    curl_easy_cleanup(curl);
  }
  return (result ? content : "");
}

std::string html_to_text(const std::string &html) {
  std::string result = html;

  result =
      std::regex_replace(result, std::regex("<script[^>]*>[^<]*</script>"), "");
  result =
      std::regex_replace(result, std::regex("<style[^>]*>[^<]*</style>"), "");
  result = std::regex_replace(result, std::regex("<[^>]+>"), " ");
  result = std::regex_replace(result, std::regex("\\s+"), " ");
  result = std::regex_replace(result, std::regex("^\\s+|\\s+$"), "");
  return result;
}

std::string get_webpage_text(const std::string &url) {
  std::string html_code = download_page(url);
  return html_to_text(html_code);
}

std::vector<std::string> get_tags_to_array(const std::string &html,
                                           const std::string &tag_name) {
  std::vector<std::string> result;
  std::string open_tag = "<" + tag_name;
  std::string closing_tag = "</" + tag_name + ">";

  size_t pos = 0;

  while (pos < html.length()) {
    // Find opening tag
    size_t start = html.find(open_tag, pos);
    if (start == std::string::npos)
      break;

    // find the end of opening tag
    size_t end = html.find(">", start);
    if (end == std::string::npos)
      break;

    bool is_self_closing =
        (html[end - 1] == '/') || // format: <img/>
        (tag_name == "img" || tag_name == "br" || tag_name == "hr" ||
         tag_name == "input" || tag_name == "meta" || tag_name == "link" ||
         tag_name == "base");

    if (is_self_closing) {
      result.push_back(html.substr(start, end - start + 1));
      pos = end + 1;
    } else {
      size_t close_start = html.find(closing_tag, end);
      if (close_start == std::string::npos)
        break;
      size_t length = closing_tag.length() + close_start - start;
      result.push_back(html.substr(start, length));
      pos = close_start + closing_tag.length();
    }
  }
  return result;
}

std::vector<std::string> find_elements_by_class(const std::string &html,
                                                const std::string &class_name) {
  std::vector<std::string> result;
  std::regex class_pattern("class\\s*=\\s*\"[^\"]*" + class_name + "[^\"]*\"");
  size_t pos = 0;

  while ((pos = html.find("<", pos)) != std::string::npos) {
    size_t end_pos = html.find(">", pos);
    if (end_pos == std::string::npos)
      break;

    std::string tag = html.substr(pos, end_pos - pos + 1);

    if (std::regex_search(tag, class_pattern)) {
      size_t elementEnd = pos;
      std::string elementStart = tag.substr(1, tag.find(" ") - 1);
      std::string closingTag = "</" + elementStart + ">";
      size_t closePos = html.find(closingTag, end_pos);
      if (closePos != std::string::npos) {
        std::string element =
            html.substr(pos, closePos + closingTag.length() - pos);
        result.push_back(element);
      }
    }

    pos = end_pos + 1;
  }

  return result;
}

std::string find_element_by_id(const std::string &html, const std::string &id) {
  std::regex id_pattern("id\\s*=\\s*\"" + id + "\"");
  size_t pos = 0;

  while ((pos = html.find("<", pos)) != std::string::npos) {
    size_t end_pos = html.find(">", pos);
    if (end_pos == std::string::npos)
      break;

    std::string tag = html.substr(pos, end_pos - pos + 1);
    if (std::regex_search(tag, id_pattern)) {
      size_t elementEnd = pos;
      std::string elementStart = tag.substr(1, tag.find(" ") - 1);
      std::string closingTag = "</" + elementStart + ">";
      size_t closePos = html.find(closingTag, end_pos);

      if (closePos != std::string::npos) {
        std::string element = html.substr(pos, closePos + closingTag.length() - pos);
        return get_element_text(element);
      }
    }
    pos = end_pos + 1;
  }

  return ""; // Return empty string if element not found
}

std::vector<std::string>
find_elements_by_attr_val(const std::string &html, const std::string &attr_name,
                          const std::string &attr_val) {
  std::regex attr_pattern(attr_name + "\\s*=\\s*([\"]?)\\b" + attr_val +
                          "\\b([\"]?)");
  size_t pos = 0;

  std::vector<std::string> res;

  while ((pos = html.find("<", pos)) != std::string::npos) {
    size_t end_pos = html.find(">", pos);
    if (end_pos == std::string::npos)
      break;

    std::string tag = html.substr(pos, end_pos - pos + 1);
    if (std::regex_search(tag, attr_pattern)) {
      size_t elementEnd = pos;
      std::string elementStart = tag.substr(1, tag.find(" ") - 1);

      bool is_self_closing = (html[end_pos - 1] == '/') || // format: <img/>
                             (elementStart == "img" || elementStart == "br" ||
                              elementStart == "hr" || elementStart == "input" ||
                              elementStart == "meta" ||
                              elementStart == "link" || elementStart == "base");

      if (is_self_closing) {
        res.push_back(html.substr(pos, end_pos - pos + 1));
      } else {
        std::string closingTag = "</" + elementStart + ">";
        size_t closePos = html.find(closingTag, end_pos);
        if (closePos != std::string::npos) {
          res.push_back(html.substr(pos, closePos + closingTag.length() - pos));
        }
      }
    }
    pos = end_pos + 1;
  }

  return res;
}

std::string get_element_text(const std::string &html, const std::string &tag) {
  std::string text = html;
  text = std::regex_replace(text, std::regex("<script[^>]*>[^<]*</script>"), "");
  text = std::regex_replace(text, std::regex("<style[^>]*>[^<]*</style>"), "");
  text = std::regex_replace(text, std::regex("<[^>]+>"), " ");
  text = std::regex_replace(text, std::regex("\\s+"), " ");
  text = std::regex_replace(text, std::regex("^\\s+|\\s+$"), "");

  return text;
} // namespace cnp


