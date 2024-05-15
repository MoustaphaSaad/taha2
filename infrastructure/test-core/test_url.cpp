#include <doctest/doctest.h>

#include <core/Url.h>
#include <core/Mallocator.h>

TEST_CASE("basic core::Url::parse test")
{
	core::Mallocator allocator;

	SUBCASE("basic google")
	{
		auto parsedUrl = core::Url::parse("http://www.google.com"_sv, &allocator);
		REQUIRE(parsedUrl.isError() == false);
		auto url = parsedUrl.value();
		REQUIRE(url.scheme() == "http"_sv);
		REQUIRE(url.host() == "www.google.com"_sv);
	}

	SUBCASE("basic google with simple path")
	{
		auto parsedUrl = core::Url::parse("http://www.google.com/"_sv, &allocator);
		REQUIRE(parsedUrl.isError() == false);
		auto url = parsedUrl.value();
		REQUIRE(url.scheme() == "http"_sv);
		REQUIRE(url.host() == "www.google.com"_sv);
		REQUIRE(url.path() == "/"_sv);
	}

	SUBCASE("basic google with path")
	{
		auto parsedUrl = core::Url::parse("http://www.google.com/file%20one%26two"_sv, &allocator);
		REQUIRE(parsedUrl.isError() == false);
		auto url = parsedUrl.value();
		REQUIRE(url.scheme() == "http"_sv);
		REQUIRE(url.host() == "www.google.com"_sv);
		REQUIRE(url.path() == "/file one&two"_sv);
	}

	SUBCASE("basic url with user")
	{
		auto parsedUrl = core::Url::parse("ftp://webmaster@www.google.com/"_sv, &allocator);
		REQUIRE(parsedUrl.isError() == false);
		auto url = parsedUrl.value();
		REQUIRE(url.scheme() == "ftp"_sv);
		REQUIRE(url.user() == "webmaster"_sv);
		REQUIRE(url.host() == "www.google.com"_sv);
		REQUIRE(url.path() == "/"_sv);
	}

	SUBCASE("basic url with user and password")
	{
		auto parsedUrl = core::Url::parse("ftp://john%20doe@www.google.com/"_sv, &allocator);
		REQUIRE(parsedUrl.isError() == false);
		auto url = parsedUrl.value();
		REQUIRE(url.scheme() == "ftp"_sv);
		REQUIRE(url.user() == "john doe"_sv);
		REQUIRE(url.host() == "www.google.com"_sv);
		REQUIRE(url.path() == "/"_sv);
	}

	SUBCASE("basic url with simple query")
	{
		auto parsedUrl = core::Url::parse("http://www.google.com/?"_sv, &allocator);
		REQUIRE(parsedUrl.isError() == false);
		auto url = parsedUrl.value();
		REQUIRE(url.scheme() == "http"_sv);
		REQUIRE(url.host() == "www.google.com"_sv);
		REQUIRE(url.path() == "/"_sv);
		REQUIRE(url.query().count() == 0);
	}

	SUBCASE("google with query")
	{
		auto parsedUrl = core::Url::parse("http://www.google.com/?foo=bar?"_sv, &allocator);
		REQUIRE(parsedUrl.isError() == false);
		auto url = parsedUrl.value();
		REQUIRE(url.scheme() == "http"_sv);
		REQUIRE(url.host() == "www.google.com"_sv);
		REQUIRE(url.path() == "/"_sv);
		REQUIRE(url.query().get(url.query().find("foo"_sv)) == "bar?"_sv);
	}

	SUBCASE("google with complex query")
	{
		auto parsedUrl = core::Url::parse("http://www.google.com/?q=go%20language"_sv, &allocator);
		REQUIRE(parsedUrl.isError() == false);
		auto url = parsedUrl.value();
		REQUIRE(url.scheme() == "http"_sv);
		REQUIRE(url.host() == "www.google.com"_sv);
		REQUIRE(url.path() == "/"_sv);
		REQUIRE(url.query().get(url.query().find("q"_sv)) == "go language"_sv);
	}

	SUBCASE("basic mailto")
	{
		auto parsedUrl = core::Url::parse("mailto:webmaster@golang.org"_sv, &allocator);
		REQUIRE(parsedUrl.isError() == false);
		auto url = parsedUrl.value();
		REQUIRE(url.scheme() == "mailto"_sv);
		REQUIRE(url.path() == "webmaster@golang.org"_sv);
	}

	SUBCASE("ipv6")
	{
		auto parsedUrl = core::Url::parse("http://[2b01:e34:ef40:7730:8e70:5aff:fefe:edac]:8080/foo"_sv, &allocator);
		REQUIRE(parsedUrl.isError() == false);
		auto url = parsedUrl.value();
		REQUIRE(url.scheme() == "http"_sv);
		REQUIRE(url.host() == "2b01:e34:ef40:7730:8e70:5aff:fefe:edac"_sv);
		REQUIRE(url.port() == "8080"_sv);
		REQUIRE(url.path() == "/foo"_sv);
	}

	SUBCASE("url with fragment")
	{
		auto parsedUrl = core::Url::parse("http://www.google.com/?#id_token=abcd%20efh"_sv, &allocator);
		REQUIRE(parsedUrl.isError() == false);
		auto url = parsedUrl.value();
		REQUIRE(url.scheme() == "http"_sv);
		REQUIRE(url.host() == "www.google.com"_sv);
		REQUIRE(url.path() == "/"_sv);
		REQUIRE(url.query().count() == 0);
		REQUIRE(url.fragment() == "id_token=abcd efh"_sv);
	}

	SUBCASE("path")
	{
		auto parsedUrl = core::Url::parse("/path/to/file"_sv, &allocator);
		REQUIRE(parsedUrl.isError() == false);
		auto url = parsedUrl.value();
		REQUIRE(url.path() == "/path/to/file"_sv);
	}

	SUBCASE("filename")
	{
		auto parsedUrl = core::Url::parse("filename"_sv, &allocator);
		REQUIRE(parsedUrl.isError() == false);
		auto url = parsedUrl.value();
		REQUIRE(url.path() == "filename"_sv);
	}

	SUBCASE("construct url")
	{
		auto parsedUrl = core::Url::parse("https://users.moustapha.xyz/account/login"_sv, &allocator);
		REQUIRE(parsedUrl.isError() == false);
		auto url = parsedUrl.value();
		REQUIRE(url.scheme() == "https"_sv);
		REQUIRE(url.host() == "users.moustapha.xyz"_sv);
		REQUIRE(url.path() == "/account/login"_sv);

		url.query().add("scope"_sv, "openid profile"_sv);
		url.query().add("client_id"_sv, "wallet"_sv);
		url.query().add("response_type"_sv, "id_token"_sv);
		url.query().add("nonce"_sv, "zhw87kq4u5drfiwkg"_sv);
		url.query().add("redirect_uri"_sv, "http://localhost:8080/"_sv);
		url.query().add("response_mode"_sv, "form_post"_sv);

		auto result = url.toString(&allocator);
		REQUIRE(result.isError() == false);
		auto str = result.releaseValue();
		REQUIRE(str == "https://users.moustapha.xyz/account/login?scope=openid+profile&client_id=wallet&response_type=id_token&nonce=zhw87kq4u5drfiwkg&redirect_uri=http%3A%2F%2Flocalhost%3A8080%2F&response_mode=form_post"_sv);
	}

	SUBCASE("query element encoding")
	{
		auto token = ": / ? # [ ] @ ! $ & ' ( ) * + , ; = %"_sv;
		auto encoded_string = core::Url::encodeQueryElement(token, &allocator);
		REQUIRE(encoded_string == "%3A+%2F+%3F+%23+%5B+%5D+%40+%21+%24+%26+%27+%28+%29+%2A+%2B+%2C+%3B+%3D+%25"_sv);
	}
}
