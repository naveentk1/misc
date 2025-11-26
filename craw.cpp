//============================================================================
// Name        : OctaneCrawler_NoBoost.cpp
// Author      : Berlin Brown (modified)
// Version     : No Boost Dependencies
// Description : Simple web crawler using only C++ standard library
//============================================================================

#include <iostream>
#include <string>
#include <fstream>
#include <regex>
#include <algorithm>
#include <set>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

using namespace std;

const int DELAY = 2;
const int MAXRECV = 140 * 1024;
const std::string WRITE_DIR_PATH = "/tmp/octane_bot_store";

std::set<std::string> visited_urls;
int max_pages = 10;
int pages_crawled = 0;

// Helper function to convert string to lowercase
std::string to_lower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

class WebPage {
public:
    std::string hostname;
    std::string page;

    WebPage() : hostname(""), page("") {}

    std::string parseHttp(const std::string& str) {
        std::regex re("https?://([^/]+)/?.*", std::regex::icase);
        std::smatch matches;
        if (std::regex_match(str, matches, re)) {
            return to_lower(matches[1].str());
        }
        return "";
    }

    void parseHref(const std::string& orig_host, const std::string& str) {
        std::regex re("https?://([^/]+)/(.*)", std::regex::icase);
        std::smatch matches;
        if (std::regex_match(str, matches, re)) {
            hostname = to_lower(matches[1].str());
            page = matches[2].str();
        } else {
            hostname = orig_host;
            page = "";
        }
    }

    void parse(const std::string& orig_host, const std::string& hrf) {
        std::string hst = parseHttp(hrf);
        if (!hst.empty()) {
            parseHref(hst, hrf);
        } else {
            hostname = orig_host;
            page = hrf;
        }
        if (page.empty()) {
            page = "/";
        }
    }
};

std::string request(const std::string& host, const std::string& path) {
    std::string req = "GET " + path + " HTTP/1.1\r\n";
    req += "Host: " + host + "\r\n";
    req += "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
    req += "User-Agent: Mozilla/5.0 (compatible; octanebot/1.0)\r\n";
    req += "Connection: close\r\n\r\n";
    return req;
}

std::string clean_href(const std::string& host, const std::string& path) {
    std::string full_url = host + "/" + path;
    std::regex rmv_all("[^a-zA-Z0-9]");
    return std::regex_replace(full_url, rmv_all, "_");
}

bool crawl(const std::string& host, const std::string& path) {
    if (pages_crawled >= max_pages) {
        cout << "Reached maximum page limit (" << max_pages << ")" << endl;
        return false;
    }

    std::string url_key = host + path;
    if (visited_urls.find(url_key) != visited_urls.end()) {
        cout << "Already visited: " << host << path << endl;
        return false;
    }
    visited_urls.insert(url_key);
    pages_crawled++;

    const int port = 80;
    int m_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (m_sock < 0) {
        cerr << "ERROR: Socket creation failed" << endl;
        return false;
    }

    int on = 1;
    if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on)) == -1) {
        cerr << "ERROR: setsockopt failed" << endl;
        close(m_sock);
        return false;
    }

    // Parse host:port if present
    std::string actual_host = host;
    int actual_port = port;
    size_t colon_pos = host.find(':');
    if (colon_pos != std::string::npos) {
        actual_host = host.substr(0, colon_pos);
        actual_port = std::stoi(host.substr(colon_pos + 1));
    }

    struct hostent *server = gethostbyname(actual_host.c_str());
    if (server == NULL) {
        cerr << "ERROR: No such host: " << actual_host << endl;
        close(m_sock);
        return false;
    }

    struct sockaddr_in m_addr;
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(actual_port);
    memcpy(&m_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(m_sock, (struct sockaddr*)&m_addr, sizeof(m_addr)) < 0) {
        cerr << "ERROR: Connection failed to " << actual_host << ":" << actual_port << endl;
        cerr << "Error: " << strerror(errno) << endl;
        close(m_sock);
        return false;
    }

    cout << "✓ Connected to " << actual_host << ":" << actual_port << endl;

    std::string req = request(actual_host, path);
    if (send(m_sock, req.c_str(), req.size(), 0) < 0) {
        cerr << "ERROR: Send failed" << endl;
        close(m_sock);
        return false;
    }

    cout << "✓ Request sent: GET " << path << endl;

    std::string recv_data;
    char buf[MAXRECV];
    int total_received = 0;

    while (true) {
        memset(buf, 0, MAXRECV);
        int status = recv(m_sock, buf, MAXRECV - 1, 0);
        if (status <= 0) break;
        recv_data.append(buf, status);
        total_received += status;
    }

    close(m_sock);

    if (total_received == 0) {
        cerr << "ERROR: No data received" << endl;
        return false;
    }

    cout << "✓ Received " << total_received << " bytes" << endl;

    std::string filename = WRITE_DIR_PATH + "/" + clean_href(host, path) + ".html";
    ofstream outfile(filename);
    if (!outfile.is_open()) {
        cerr << "ERROR: Could not open file: " << filename << endl;
        return false;
    }
    outfile << recv_data;
    outfile.close();
    cout << "✓ Saved to: " << filename << endl;

    // Extract body
    size_t body_pos = recv_data.find("\r\n\r\n");
    std::string body = (body_pos != std::string::npos) ? recv_data.substr(body_pos + 4) : recv_data;

    // Remove newlines
    body.erase(std::remove(body.begin(), body.end(), '\r'), body.end());
    body.erase(std::remove(body.begin(), body.end(), '\n'), body.end());

    // Find all links
    std::regex href_regex("href\\s*=\\s*[\"']([^\"']+)[\"']", std::regex::icase);
    auto links_begin = std::sregex_iterator(body.begin(), body.end(), href_regex);
    auto links_end = std::sregex_iterator();

    int links_found = 0;
    for (std::sregex_iterator i = links_begin; i != links_end; ++i) {
        std::string href = (*i)[1].str();
        
        if (href.empty() || href[0] == '#' || 
            href.find("javascript:") != std::string::npos ||
            href.find("mailto:") != std::string::npos) {
            continue;
        }

        links_found++;
        WebPage wp;
        wp.parse(host, href);
        
        cout << "  → Link: " << wp.hostname << wp.page << endl;

        if (wp.hostname == host) {
            sleep(DELAY);
            crawl(wp.hostname, wp.page);
        }
    }
    
    cout << "✓ Found " << links_found << " links\n" << endl;
    return true;
}

int main(int argc, char* argv[]) {
    cout << "========================================" << endl;
    cout << "   Octane Crawler (No Boost)" << endl;
    cout << "========================================" << endl;
    
    system(("mkdir -p " + WRITE_DIR_PATH).c_str());
    
    std::string target_host = "localhost";
    std::string target_path = "/";
    
    // Allow command-line arguments
    if (argc >= 2) {
        target_host = argv[1];
    }
    if (argc >= 3) {
        target_path = argv[2];
    }
    
    cout << "\nTarget: http://" << target_host << target_path << endl;
    cout << "Output: " << WRITE_DIR_PATH << endl;
    cout << "Max pages: " << max_pages << endl;
    cout << "Delay: " << DELAY << " sec\n" << endl;

    crawl(target_host, target_path);
    
    cout << "\n========================================" << endl;
    cout << "Crawl complete!" << endl;
    cout << "Pages: " << pages_crawled << endl;
    cout << "Output: " << WRITE_DIR_PATH << endl;
    cout << "========================================" << endl;
    
    return 0;
}