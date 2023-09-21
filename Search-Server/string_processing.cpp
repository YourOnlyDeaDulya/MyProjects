#include "string_processing.h"

using namespace std;

vector<string_view> SplitIntoWordsView(string_view stop_words_text) {
    string_view str = stop_words_text;
    vector<string_view> result;
    if (str.empty())
    {
        return result;
    }

    str.remove_prefix(str.find_first_not_of(" ") == str.npos ? str.size() : str.find_first_not_of(" "));

    while (!str.empty()) {
        int64_t space = str.find(' ');
        result.push_back(str.find_first_not_of(" ") == str.npos ? str.substr(0) : str.substr(0, space));
        str.remove_prefix(space == static_cast<int64_t>(str.npos) ? str.size() : space);
        str.remove_prefix(str.find_first_not_of(" ") == str.npos ? str.size() : str.find_first_not_of(" "));
    }

    return result;
}