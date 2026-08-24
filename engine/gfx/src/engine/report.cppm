export module vw.gfx:engine.report;

import std;

import vw.core;

export namespace vw::gfx {

// Отчёт прогона: дерево секций с именованными значениями, которое печатается
// двумя способами — человеку таблицей, машине деревом JSON.
//
// Раньше каждая сцена печатала свой блок сама, из деструктора приложения и
// прямо в stdout, то есть уже после того, как движок записал файл отчёта.
// Числа сцены — то, ради чего сцена и существует, — в отчёт не попадали вовсе,
// и снять их можно было только перехватом потока.
class report_section {
public:
    explicit report_section(std::string name) : name_{std::move(name)} {}

    auto value(std::string_view key, float64 number, int32 precision = 3) -> report_section& {
        entries_.push_back({std::string{key}, std::format("{:.{}f}", number, precision), false});
        return *this;
    }

    auto value(std::string_view key, int64 number) -> report_section& {
        entries_.push_back({std::string{key}, std::format("{}", number), false});
        return *this;
    }

    auto value(std::string_view key, uint64 number) -> report_section& {
        entries_.push_back({std::string{key}, std::format("{}", number), false});
        return *this;
    }

    auto value(std::string_view key, bool flag) -> report_section& {
        entries_.push_back({std::string{key}, flag ? "true" : "false", false});
        return *this;
    }

    auto value(std::string_view key, std::string_view text) -> report_section& {
        entries_.push_back({std::string{key}, std::string{text}, true});
        return *this;
    }

    // Подсекция для того, что само по себе список: у сетки источников два
    // списка, и среднее по обоим сразу не описывает ни одного.
    // Ссылку на секцию держат, пока её заполняют, поэтому хранилище обязано
    // переживать вставку следующей: у list ссылки на элементы это переживают, у
    // vector — нет. list ещё и единственный из трёх, кому стандарт разрешает
    // неполный тип, а секция здесь ссылается сама на себя.
    auto sub(std::string_view name) -> report_section& {
        subs_.push_back(report_section{std::string{name}});
        return subs_.back();
    }

    [[nodiscard]] auto name() const -> const std::string& {
        return name_;
    }

    [[nodiscard]] auto empty() const -> bool {
        return entries_.empty() && subs_.empty();
    }

    auto write_text(std::string& out, int32 depth) const -> void;
    auto write_json(std::string& out, int32 depth) const -> void;

private:
    struct entry {
        std::string key;
        std::string text;
        bool quoted = false;
    };

    std::string name_;
    std::vector<entry> entries_;
    std::list<report_section> subs_;
};

class report {
public:
    auto section(std::string_view name) -> report_section& {
        sections_.push_back(report_section{std::string{name}});
        return sections_.back();
    }

    [[nodiscard]] auto empty() const -> bool {
        return sections_.empty();
    }

    [[nodiscard]] auto to_text() const -> std::string;
    [[nodiscard]] auto to_json() const -> std::string;

private:
    std::deque<report_section> sections_;
};

}  // namespace vw::gfx
