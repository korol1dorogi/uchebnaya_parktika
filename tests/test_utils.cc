#include <gtest/gtest.h>

#include <cstdlib>
#include <string>

#include "Env.h"
#include "Validation.h"

namespace
{
void setEnv(const char *key, const char *value)
{
#ifdef _WIN32
    _putenv_s(key, value);
#else
    setenv(key, value, 1);
#endif
}
}  // namespace

TEST(EnvOr, DefaultWhenUnset)
{
    EXPECT_EQ(util::envOr("PRAKTIKA_DEFINITELY_UNSET_XYZ", "def"), "def");
}

TEST(EnvOr, ReturnsValueWhenSet)
{
    setEnv("PRAKTIKA_TEST_VAR", "hello");
    EXPECT_EQ(util::envOr("PRAKTIKA_TEST_VAR", "def"), "hello");
}

TEST(Validation, LoginValid)
{
    EXPECT_TRUE(util::isValidLogin("abc"));
    EXPECT_TRUE(util::isValidLogin("user_123"));
    EXPECT_TRUE(util::isValidLogin(std::string(32, 'a')));
}

TEST(Validation, LoginInvalid)
{
    EXPECT_FALSE(util::isValidLogin(""));               // пустой
    EXPECT_FALSE(util::isValidLogin("ab"));             // короткий
    EXPECT_FALSE(util::isValidLogin(std::string(33, 'a')));  // длинный
    EXPECT_FALSE(util::isValidLogin("bad login"));      // пробел
    EXPECT_FALSE(util::isValidLogin("dash-no"));        // дефис
}

TEST(Validation, Password)
{
    EXPECT_TRUE(util::isValidPassword("pass"));
    EXPECT_TRUE(util::isValidPassword(std::string(128, 'x')));
    EXPECT_FALSE(util::isValidPassword("abc"));               // короткий
    EXPECT_FALSE(util::isValidPassword(std::string(129, 'x')));  // длинный
}
