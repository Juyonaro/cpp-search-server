#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server)
    : search_server_(search_server) {}

int RequestQueue::GetNoResultRequests() const {
    return unprocessed_requests_;
}