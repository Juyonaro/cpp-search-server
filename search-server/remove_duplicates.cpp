#include "remove_duplicates.h"

#include <iostream>

void RemoveDuplicates(SearchServer& search_server) {
    // Множество ID док-тов дубликатов
    std::set<int> duplicates;
    // Множество слов док-тов
    std::set<std::set<std::string>> documents;
    
    for (const int documents_current : search_server) {
        // Множество слов в док-те
        std::set<std::string> words;
        
        const auto& word_freq = search_server.GetWordFrequencies(documents_current);
        
        for (const auto [word, _] : word_freq) {
            words.emplace(word);
        }
        
        // Если слова идентичные, значит это дубликат
        if (documents.count(words)) {
            duplicates.emplace(documents_current);
        } else { // Иначе добавляем новый уникальный док-т
            documents.emplace(words);
        }
    }
    
    for (auto id : duplicates) {
        std::cout << "Found duplicate document id "s << id << std::endl;
        search_server.RemoveDocument(id);
    }
}
