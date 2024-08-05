#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <curl/curl.h>
#include <gumbo.h>

// Callback function to write data from the response
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to initialize CURL and fetch the webpage content
std::string fetch_webpage(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string read_buffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);
        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return read_buffer;
}

// Recursive function to traverse the HTML tree and extract data
void search_for_products(GumboNode* node, std::vector<std::tuple<std::string, std::string, std::string>>& products) {
    if (node->type != GUMBO_NODE_ELEMENT) return;

    GumboAttribute* attr;
    if (node->v.element.tag == GUMBO_TAG_DIV) {
        if ((attr = gumbo_get_attribute(&node->v.element.attributes, "class")) &&
            std::string(attr->value) == "product") {

            std::string name, price, rating;
            for (GumboNode* child : node->v.element.children) {
                if (child->type == GUMBO_NODE_ELEMENT) {
                    GumboAttribute* child_attr;
                    if (child->v.element.tag == GUMBO_TAG_H2 && (child_attr = gumbo_get_attribute(&child->v.element.attributes, "class")) && std::string(child_attr->value) == "product-name") {
                        name = child->v.element.text.text;
                    } else if (child->v.element.tag == GUMBO_TAG_SPAN && (child_attr = gumbo_get_attribute(&child->v.element.attributes, "class")) && std::string(child_attr->value) == "product-price") {
                        price = child->v.element.text.text;
                    } else if (child->v.element.tag == GUMBO_TAG_DIV && (child_attr = gumbo_get_attribute(&child->v.element.attributes, "class")) && std::string(child_attr->value) == "product-rating") {
                        rating = child->v.element.text.text;
                    }
                }
            }

            if (!name.empty() && !price.empty() && !rating.empty()) {
                products.push_back(std::make_tuple(name, price, rating));
            }
        }
    }

    for (size_t i = 0; i < node->v.element.children.length; ++i) {
        search_for_products(static_cast<GumboNode*>(node->v.element.children.data[i]), products);
    }
}

int main() {
    std::string url = "https://example.com/products";
    std::string html_content = fetch_webpage(url);

    GumboOutput* output = gumbo_parse(html_content.c_str());
    std::vector<std::tuple<std::string, std::string, std::string>> products;
    search_for_products(output->root, products);
    gumbo_destroy_output(&kGumboDefaultOptions, output);

    std::ofstream file("products.csv");
    file << "Product Name,Price,Rating\n";
    for (const auto& product : products) {
        file << std::get<0>(product) << "," << std::get<1>(product) << "," << std::get<2>(product) << "\n";
    }

    std::cout << "Data has been written to products.csv" << std::endl;
    return 0;
}