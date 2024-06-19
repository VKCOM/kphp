#include <format>
#include <iostream>
#include <iterator>
int main() {
    std::format_to(std::ostreambuf_iterator<char>(std::cout), "Hello!\n");
}
