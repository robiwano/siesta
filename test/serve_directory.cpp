#include <gtest/gtest.h>
#include <siesta/client.h>
#include <siesta/server.h>
using namespace siesta;

#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;

#include <string.h>

namespace
{
    constexpr auto j =
        R"({
    "some": 1,
    "content": ["yay"]
})";

    struct TempFile {
        fs::path workdir;
        fs::path file;

        TempFile(const std::string& file_name, const void* content, size_t size)
        {
            auto tmp_path = fs::temp_directory_path();
            workdir       = tmp_path / "serve_directory";
            file          = workdir / fs::path(file_name);
            auto parent   = file.parent_path();
            if (!fs::exists(parent)) {
                fs::create_directories(parent);
            }
            std::ofstream os(file,
                             std::ios_base::trunc | std::ios_base::binary);
            os.write((char*)content, size);
        }
        ~TempFile()
        {
            while (!fs::equivalent(file, fs::temp_directory_path())) {
                fs::remove(file);
                file = file.parent_path();
            }
        }
        std::string path() const
        {
            return fs::relative(file, workdir).generic_string();
        }
        std::string directory() const { return workdir.generic_string(); }
    };

    static TempFile file("testdir/subdir/file.txt", j, strlen(j));

}  // namespace

TEST(siesta, serve_root_uri)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer("http://127.0.0.1:8080"));
    EXPECT_NO_THROW(server->start());

    server::TokenHolder holder;
    EXPECT_NO_THROW(holder += server->addDirectory("/", file.directory()));

    auto f = client::getRequest("http://127.0.0.1:8080/" + file.path(), 1000);

    std::string result;
    EXPECT_NO_THROW(result = f.get());
    EXPECT_EQ(result, j);
}

TEST(siesta, serve_non_root_uri)
{
    std::shared_ptr<server::Server> server;
    EXPECT_NO_THROW(server = server::createServer("http://127.0.0.1:8080"));
    EXPECT_NO_THROW(server->start());

    server::TokenHolder holder;
    EXPECT_NO_THROW(holder += server->addDirectory("/docs", file.directory()));

    auto f =
        client::getRequest("http://127.0.0.1:8080/docs/" + file.path(), 1000);

    std::string result;
    EXPECT_NO_THROW(result = f.get());
    EXPECT_EQ(result, j);
}
