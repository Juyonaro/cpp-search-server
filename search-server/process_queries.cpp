#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server, const std::vector<std::string>& queries) {
    
    std::vector<std::vector<Document>> result_queries(queries.size());
    
    std::transform(std::execution::par, queries.begin(), queries.end(), 
                   result_queries.begin(),
                   [&search_server](const std::string& query) {
                       return search_server.FindTopDocuments(query);
                   });
    
    return result_queries;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server, const std::vector<std::string>& queries) {
    
    std::vector<Document> result_queries;
    
    for (const auto& document : ProcessQueries(search_server, queries)) {
        result_queries.insert(result_queries.end(), document.begin(), document.end());
    }
    
    return result_queries;
}