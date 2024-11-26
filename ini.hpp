#pragma once
#include <memory> // std::shared_ptr
#include <string> // std::string
#include <vector> // std::vector
#include <string_view> // std::string_view
#include <cstdio> // std::snprintf
#include <cstdint> // SIZE_MAX, std::int32_t, std::int64_t
#include <cstdlib> // std::atoi, std::atoll, std::strtof, std::atof
#include <cstddef> // std::size_t
#include <cinttypes> // PRId32, PRId64

enum class ini_error {
	none,
	section_missing, // field has no section
	section_close, // section not closed
	section_empty, // section is empty
	assignment_symbol, // expected assignment
	key_empty // key is empty
};
class ini_comment {
public:
	std::string get_comment() const;
	std::vector<std::string> get_comments() const;
	void set_comment(std::string_view value);
	void set_comments(std::vector<std::string_view> values);
protected:
	std::string comment_;
	std::vector<std::string> comments_;
};
class ini_option final : public ini_comment {
friend class ini;
friend class ini_section;
public:
	bool get_bool(bool default_value) const;
	std::int32_t get_int32(std::int32_t default_value) const;
	std::int64_t get_int64(std::int64_t default_value) const;
	float get_float32(float default_value) const;
	double get_float64(double default_value) const;
	std::string get_string(std::string_view default_value) const;
	void set_bool(bool value);
	void set_int32(std::int32_t value);
	void set_int64(std::int64_t value);
	void set_float32(float value);
	void set_float64(double value);
	void set_string(std::string_view value);
private:
	std::string name_;
	std::string value_;
};
class ini_section final : public ini_comment {
friend class ini;
public:
	std::shared_ptr<ini_option> option(std::string_view name);
	bool has_option(std::string_view name) const;
	void remove_option(std::string_view name);
private:
	std::string name_;
	std::vector<std::shared_ptr<ini_option>> options_;
};
class ini final {
public:
	std::shared_ptr<ini_section> section(std::string_view name);
	bool has_section(std::string_view name) const;
	void remove_section(std::string_view name);
	void clear();
	static ini_error parse(std::string_view text, ini &ini);
	std::string to_string() const;
private:
	std::vector<std::shared_ptr<ini_section>> sections_;
};
