#include "string_processing.h"

#include <algorithm>

std::vector<std::string_view> SplitIntoWords(const std::string_view text) {
    std::vector<std::string_view> words;

    // Находим позицию первого непробельного символа
    auto start_pos = text.find_first_not_of(' ');
    while (start_pos != std::string_view::npos) {
        // Находим позицию символа пробела после непробельного символа
        auto end_pos = text.find_first_of(' ', start_pos);
        // Извлекаем слово между позициями и добавляем в вектор слов
        words.emplace_back(text.substr(start_pos, end_pos - start_pos));
        
        // Находим позицию следующего непробельного символа
        start_pos = text.find_first_not_of(' ', end_pos);
    }

    return words;
}