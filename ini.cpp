#include "ini.hpp"

static void ini_escape_quoted(std::string_view input, std::string &output) {
	for (char character : input) {
		switch (character) {
		case '\t':
			output.append("\\t", 2);
			break;

		case '\r':
			output.append("\\r", 2);
			break;

		case '\n':
			output.append("\\n", 2);
			break;

		default:
			output.push_back(character);
			break;
		}
	}
}

static void ini_unescape_quoted(std::string &string) {
	std::size_t number = 0;

	for (; number < string.length(); number++) {
		if ('\t' != string[number] && ' ' != string[number]) {
			break;
		}
	}

	if (number) {
		string.erase(0, number);
	}

	for (number = string.length(); number; number--) {
		std::size_t index = number - 1;

		if ('\t' != string[index] && ' ' != string[index]) {
			break;
		}
	}

	if (string.length() != number) {
		string.erase(number, SIZE_MAX);
	}

	for (number = 0; number < string.length(); number++) {
		char character = string[number];

		if ('\\' == character && number < string.length()) {
			character = string[number + 1];

			if ('t' == character) {
				string.erase(number, 2);
				string.insert(number, 1, '\t');
			} else if ('r' == character) {
				string.erase(number, 2);
				string.insert(number, 1, '\r');
			} else if ('n' == character) {
				string.erase(number, 2);
				string.insert(number, 1, '\n');
			} else {
				number++;
			}
		}
	}
}

std::string ini_comment::get_comment() const {
	return comment_;
}

std::vector<std::string> ini_comment::get_comments() const {
	return comments_;
}

void ini_comment::set_comment(std::string_view value) {
	comment_ = value;
}

void ini_comment::set_comments(std::vector<std::string_view> values) {
	for (std::string_view value : values) {
		comments_.push_back(std::string(value));
	}
}

bool ini_option::get_bool(bool default_value) const {
	if (value_.length()) {
		if ("1" == value_ || "true" == value_ || "yes" == value_ || "on" == value_) {
			return true;
		} else if ("0" == value_ || "false" == value_ || "no" == value_ || "off" == value_) {
			return false;
		}
	}

	return default_value;
}

std::int32_t ini_option::get_int32(std::int32_t default_value) const {
	return value_.length() ? std::atoi(value_.c_str()) : default_value;
}

std::int64_t ini_option::get_int64(std::int64_t default_value) const {
	return value_.length() ? std::atoll(value_.c_str()) : default_value;
}

float ini_option::get_float32(float default_value) const {
	return value_.length() ? std::strtof(value_.c_str(), nullptr) : default_value;
}

double ini_option::get_float64(double default_value) const {
	return value_.length() ? std::atof(value_.c_str()) : default_value;
}

std::string ini_option::get_string(std::string_view default_value) const {
	return value_.length() ? value_ : std::string(default_value);
}

void ini_option::set_bool(bool value) {
	if (value) {
		value_.assign("true", 4);
	} else {
		value_.assign("false", 5);
	}
}

void ini_option::set_int32(std::int32_t value) {
	char buffer[12] = {0};
	value_.assign(buffer, std::snprintf(buffer, 12, "%" PRId32, value));
}

void ini_option::set_int64(std::int64_t value) {
	char buffer[21] = {0};
	value_.assign(buffer, std::snprintf(buffer, 21, "%" PRId64, value));
}

void ini_option::set_float32(float value) {
	char buffer[16] = {0};
	value_.assign(buffer, std::snprintf(buffer, 16, "%.8g", value));
}

void ini_option::set_float64(double value) {
	char buffer[25] = {0};
	value_.assign(buffer, snprintf(buffer, 25, "%.17g", value));
}

void ini_option::set_string(std::string_view value) {
	value_ = value;
}

std::shared_ptr<ini_option> ini_section::option(std::string_view name) {
	for (std::shared_ptr<ini_option> &option : options_) {
		if (option->name_ == name) {
			return option;
		}
	}

	std::shared_ptr<ini_option> new_option = std::make_shared<ini_option>();
	new_option->name_ = name;
	options_.emplace_back(new_option);
	return options_.back();
}

bool ini_section::has_option(std::string_view name) const {
	for (const std::shared_ptr<ini_option> &option : options_) {
		if (option->name_ == name) {
			return true;
		}
	}

	return false;
}

void ini_section::remove_option(std::string_view name) {
	std::vector<std::shared_ptr<ini_option>>::iterator iteration_end = options_.end();
	std::vector<std::shared_ptr<ini_option>>::iterator iteration_begin = options_.begin();

	while (iteration_begin != iteration_end) {
		if (iteration_begin->get()->name_ == name) {
			options_.erase(iteration_begin);
			break;
		}

		iteration_begin++;
	}
}

std::shared_ptr<ini_section> ini::section(std::string_view name) {
	for (std::shared_ptr<ini_section> &section : sections_) {
		if (section->name_ == name) {
			return section;
		}
	}

	std::shared_ptr<ini_section> new_section = std::make_shared<ini_section>();
	new_section->name_ = name;
	sections_.emplace_back(new_section);
	return sections_.back();
}


bool ini::has_section(std::string_view name) const {
	for (const std::shared_ptr<ini_section> &section : sections_) {
		if (section->name_ == name) {
			return true;
		}
	}

	return false;
}

void ini::remove_section(std::string_view name) {
	std::vector<std::shared_ptr<ini_section>>::iterator iteration_end = sections_.end();
	std::vector<std::shared_ptr<ini_section>>::iterator iteration_begin = sections_.begin();

	while (iteration_begin != iteration_end) {
		if (iteration_begin->get()->name_ == name) {
			sections_.erase(iteration_begin);
			break;
		}

		iteration_begin++;
	}
}

void ini::clear() {
	sections_.clear();
}

ini_error ini::parse(std::string_view text, ini &ini) {
	enum class ini_state {
		start,
		comment,
		section_label,
		section_jump,
		section_comment,
		key,
		value,
		value_comment,
	} status = ini_state::start;
	bool need_lf = false;
	ini_error result = ini_error::none;
	std::shared_ptr<ini_option> option;
	std::shared_ptr<ini_section> section;
	std::string comment;
	std::string temporary;
	std::vector<std::string> comments;
	
	for (size_t text_index = 0; ; text_index++) {
		char character = text_index < text.length() ? text[text_index] : '\0';

		switch (status) {
		case ini_state::start:
			switch (character) {
			case '\0':
				goto end;

			case '\r':
				need_lf = true;
				comments.emplace_back(comment);
				break;

			case '\n':
				if (need_lf) {
					need_lf = false;
				} else {
					comments.emplace_back(comment);
				}

				break;

			case '\t': case '\f': case '\v': case ' ':
				break;

			case ';': case '#':
				status = ini_state::comment;
				break;

			case '[':
				status = ini_state::section_label;
				break;

			default:
				if (section) {
					temporary.push_back(character);
					status = ini_state::key;
				} else {
					goto section_missing_error;
				}
			}

			break;

		case ini_state::comment:
			switch (character) {
			case '\0':
				goto end;

			case '\r':
				need_lf = true;
				[[fallthrough]];

			case '\n':
				comments.emplace_back(comment);
				comment.clear();
				status = ini_state::start;
				break;

			default:
				comment.push_back(character);
			}

			break;

		case ini_state::section_label:
			switch (character) {
			case '\0': case '\r': case '\n':
				goto section_close_error;

			case ']':
				ini_unescape_quoted(temporary);

				if (temporary.empty()) {
					goto section_empty_error;
				}

				section = ini.section(temporary);
				temporary.clear();

				if (comments.size()) {
					section->comments_ = comments;
					comments.clear();
				}

				status = ini_state::section_jump;
				break;

			default:
				temporary.push_back(character);
			}

			break;

		case ini_state::section_jump:
			switch (character) {
			case '\0':
				goto end;

			case '\r':
				need_lf = true;
				[[fallthrough]];

			case '\n':
				status = ini_state::start;
				break;

			case ';': case '#':
				status = ini_state::section_comment;
				break;
			}

			break;

		case ini_state::section_comment:
			switch (character) {
			case '\r':
				need_lf = true;
				[[fallthrough]];

			case '\n':
				[[fallthrough]];

			case '\0':
				section->comment_ = comment;
				comment.clear();
				status = ini_state::start;
				break;

			default:
				comment.push_back(character);
			}

			break;

		case ini_state::key:
			switch (character) {
			case '\0': case '\r': case '\n':
				goto assignment_symbol_error;

			case '=':
				ini_unescape_quoted(temporary);

				if (temporary.empty()) {
					goto key_empty_error;
				}

				option = section->option(temporary);
				temporary.clear();

				if (comments.size()) {
					option->comments_ = comments;
					comments.clear();
				}

				status = ini_state::value;
				break;

			default:
				temporary.push_back(character);
			}

			break;

		case ini_state::value:
			switch (character) {
			case '\r':
				need_lf = true;
				[[fallthrough]];

			case '\n':
				[[fallthrough]];

			case '\0': case ';': case '#':
				ini_unescape_quoted(temporary);
				option->value_ = temporary;
				temporary.clear();

				if (';' == character || '#' == character) {
					status = ini_state::value_comment;
				} else if ('\0' == character) {
					goto end;
				} else {
					status = ini_state::start;
				}

				break;

			default:
				temporary.push_back(character);
			}

			break;

		case ini_state::value_comment:
			switch (character) {
			case '\r':
				need_lf = true;
				[[fallthrough]];

			case '\n':
				[[fallthrough]];

			case '\0':
				option->comment_ = comment;
				comment.clear();
				status = ini_state::start;
				break;

			default:
				comment.push_back(character);
			}

			break;
		}
	}

	end:
		return result;

	section_missing_error:
		result = ini_error::section_missing;
		goto end;

	section_close_error:
		result = ini_error::section_close;
		goto end;

	section_empty_error:
		result = ini_error::section_empty;
		goto end;

	assignment_symbol_error:
		result = ini_error::assignment_symbol;
		goto end;

	key_empty_error:
		result = ini_error::key_empty;
		goto end;
}

std::string ini::to_string() const {
	std::string result;

	for (std::shared_ptr<ini_section> section : sections_) {
		if (false == section->comments_.empty()) {
			for (std::string_view comment : section->comments_) {
				if (false == comment.empty()) {
					result.push_back(';');
					result.append(comment);
				}

				result.push_back('\n');
			}
		}

		result.push_back('[');
		ini_escape_quoted(section->name_, result);
		result.push_back(']');

		if (false == section->comment_.empty()) {
			result.append(" ;", 2);
			result.append(section->comment_);
		}

		result.push_back('\n');

		for (std::shared_ptr<ini_option> option : section->options_) {
			if (false == option->comments_.empty()) {
				for (std::string_view comment : option->comments_) {
					if (false == comment.empty()) {
						result.push_back(';');
						result.append(comment);
					}

					result.push_back('\n');
				}
			}

			ini_escape_quoted(option->name_, result);
			result.append(" = ", 3);
			ini_escape_quoted(option->value_, result);

			if (false == option->comment_.empty()) {
				result.append(" ;", 2);
				result.append(option->comment_);
			}

			result.push_back('\n');
		}
	}

	return result;
}
