#include "Json.h"

#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// ============================================================
// 构造
// ============================================================
Json::Json(bool value)
    : m_type(Type::Bool), m_bool(value)
{
}

Json::Json(double value)
    : m_type(Type::Number), m_number(value)
{
}

Json::Json(int value)
    : m_type(Type::Number), m_number(static_cast<double>(value))
{
}

Json::Json(const std::string& value)
    : m_type(Type::String), m_string(value)
{
}

Json::Json(const char* value)
    : m_type(Type::String), m_string(value != nullptr ? value : "")
{
}

Json Json::MakeArray()
{
    Json value;
    value.m_type = Type::Array;
    return value;
}

Json Json::MakeObject()
{
    Json value;
    value.m_type = Type::Object;
    return value;
}

// ============================================================
// 数组 / 对象操作
// ============================================================
void Json::Push(Json value)
{
    if (m_type != Type::Array)
    {
        m_type = Type::Array;
        m_elements.clear();
    }

    m_elements.push_back(std::move(value));
}

std::size_t Json::Size() const
{
    if (m_type == Type::Array)
    {
        return m_elements.size();
    }

    if (m_type == Type::Object)
    {
        return m_members.size();
    }

    return 0;
}

void Json::Set(const std::string& key, Json value)
{
    if (m_type != Type::Object)
    {
        m_type = Type::Object;
        m_members.clear();
    }

    for (auto& member : m_members)
    {
        if (member.first == key)
        {
            member.second = std::move(value);
            return;
        }
    }

    m_members.emplace_back(key, std::move(value));
}

bool Json::Has(const std::string& key) const
{
    if (m_type != Type::Object)
    {
        return false;
    }

    for (const auto& member : m_members)
    {
        if (member.first == key)
        {
            return true;
        }
    }

    return false;
}

const Json& Json::operator[](const std::string& key) const
{
    static const Json nullValue;

    if (m_type != Type::Object)
    {
        return nullValue;
    }

    for (const auto& member : m_members)
    {
        if (member.first == key)
        {
            return member.second;
        }
    }

    return nullValue;
}

// ============================================================
// 取值
// ============================================================
std::string Json::AsString(const std::string& fallback) const
{
    return m_type == Type::String ? m_string : fallback;
}

double Json::AsDouble(double fallback) const
{
    return m_type == Type::Number ? m_number : fallback;
}

int Json::AsInt(int fallback) const
{
    if (m_type != Type::Number)
    {
        return fallback;
    }

    return static_cast<int>(std::lround(m_number));
}

bool Json::AsBool(bool fallback) const
{
    return m_type == Type::Bool ? m_bool : fallback;
}

// ============================================================
// 序列化
// ============================================================
namespace
{
    void EscapeString(const std::string& input, std::string& output)
    {
        output.push_back('"');

        for (unsigned char ch : input)
        {
            switch (ch)
            {
            case '"':
                output += "\\\"";
                break;

            case '\\':
                output += "\\\\";
                break;

            case '\b':
                output += "\\b";
                break;

            case '\f':
                output += "\\f";
                break;

            case '\n':
                output += "\\n";
                break;

            case '\r':
                output += "\\r";
                break;

            case '\t':
                output += "\\t";
                break;

            default:
                if (ch < 0x20)
                {
                    char buffer[8] = {};
                    std::snprintf(buffer, sizeof(buffer), "\\u%04x", ch);
                    output += buffer;
                }
                else
                {
                    // UTF-8 字节原样输出，文件本身就是 UTF-8
                    output.push_back(static_cast<char>(ch));
                }
                break;
            }
        }

        output.push_back('"');
    }

    std::string NumberToString(double value)
    {
        if (!std::isfinite(value))
        {
            return "0";
        }

        char buffer[64] = {};
        const auto result = std::to_chars(
            buffer, buffer + sizeof(buffer), value);

        if (result.ec != std::errc())
        {
            return "0";
        }

        return std::string(buffer, result.ptr);
    }

    void Indent(std::string& output, int level)
    {
        output.append(static_cast<std::size_t>(level) * 2, ' ');
    }

    void DumpValue(const Json& value, bool pretty, int level,
                   std::string& output)
    {
        switch (value.GetType())
        {
        case Json::Type::Null:
            output += "null";
            return;

        case Json::Type::Bool:
            output += value.AsBool() ? "true" : "false";
            return;

        case Json::Type::Number:
            output += NumberToString(value.AsDouble());
            return;

        case Json::Type::String:
            EscapeString(value.AsString(), output);
            return;

        case Json::Type::Array:
        {
            const auto& elements = value.Elements();

            if (elements.empty())
            {
                output += "[]";
                return;
            }

            output.push_back('[');

            for (std::size_t i = 0; i < elements.size(); ++i)
            {
                if (i > 0)
                {
                    output.push_back(',');
                }

                if (pretty)
                {
                    output.push_back('\n');
                    Indent(output, level + 1);
                }

                DumpValue(elements[i], pretty, level + 1, output);
            }

            if (pretty)
            {
                output.push_back('\n');
                Indent(output, level);
            }

            output.push_back(']');
            return;
        }

        case Json::Type::Object:
        {
            const auto& members = value.Members();

            if (members.empty())
            {
                output += "{}";
                return;
            }

            output.push_back('{');

            for (std::size_t i = 0; i < members.size(); ++i)
            {
                if (i > 0)
                {
                    output.push_back(',');
                }

                if (pretty)
                {
                    output.push_back('\n');
                    Indent(output, level + 1);
                }

                EscapeString(members[i].first, output);
                output += pretty ? ": " : ":";
                DumpValue(members[i].second, pretty, level + 1, output);
            }

            if (pretty)
            {
                output.push_back('\n');
                Indent(output, level);
            }

            output.push_back('}');
            return;
        }
        }
    }
}

std::string Json::Dump(bool pretty) const
{
    std::string output;
    DumpValue(*this, pretty, 0, output);
    return output;
}

// ============================================================
// 解析
// ============================================================
namespace
{
    class Parser
    {
    public:
        explicit Parser(const std::string& text)
            : m_text(text)
        {
        }

        bool Run(Json& out, std::string& error)
        {
            SkipWhitespace();

            if (!ParseValue(out, 0))
            {
                error = MakeError(m_message);
                return false;
            }

            SkipWhitespace();

            if (m_pos != m_text.size())
            {
                error = MakeError("根元素之后还有多余内容");
                return false;
            }

            return true;
        }

    private:
        bool ParseValue(Json& out, int depth)
        {
            if (depth > 200)
            {
                m_message = "嵌套层数过深";
                return false;
            }

            if (m_pos >= m_text.size())
            {
                m_message = "内容意外结束";
                return false;
            }

            switch (m_text[m_pos])
            {
            case '{':
                return ParseObject(out, depth);

            case '[':
                return ParseArray(out, depth);

            case '"':
            {
                std::string value;

                if (!ParseString(value))
                {
                    return false;
                }

                out = Json(value);
                return true;
            }

            case 't':
                if (!MatchLiteral("true"))
                {
                    return false;
                }
                out = Json(true);
                return true;

            case 'f':
                if (!MatchLiteral("false"))
                {
                    return false;
                }
                out = Json(false);
                return true;

            case 'n':
                if (!MatchLiteral("null"))
                {
                    return false;
                }
                out = Json();
                return true;

            default:
                return ParseNumber(out);
            }
        }

        bool ParseObject(Json& out, int depth)
        {
            ++m_pos; // '{'
            SkipWhitespace();

            Json object = Json::MakeObject();

            if (m_pos < m_text.size() && m_text[m_pos] == '}')
            {
                ++m_pos;
                out = std::move(object);
                return true;
            }

            while (true)
            {
                SkipWhitespace();

                if (m_pos >= m_text.size() || m_text[m_pos] != '"')
                {
                    m_message = "对象的键必须是字符串";
                    return false;
                }

                std::string key;

                if (!ParseString(key))
                {
                    return false;
                }

                SkipWhitespace();

                if (m_pos >= m_text.size() || m_text[m_pos] != ':')
                {
                    m_message = "键之后缺少冒号";
                    return false;
                }

                ++m_pos;
                SkipWhitespace();

                Json value;

                if (!ParseValue(value, depth + 1))
                {
                    return false;
                }

                object.Set(key, std::move(value));

                SkipWhitespace();

                if (m_pos >= m_text.size())
                {
                    m_message = "对象没有闭合";
                    return false;
                }

                if (m_text[m_pos] == ',')
                {
                    ++m_pos;
                    continue;
                }

                if (m_text[m_pos] == '}')
                {
                    ++m_pos;
                    out = std::move(object);
                    return true;
                }

                m_message = "对象成员之间缺少逗号或右花括号";
                return false;
            }
        }

        bool ParseArray(Json& out, int depth)
        {
            ++m_pos; // '['
            SkipWhitespace();

            Json array = Json::MakeArray();

            if (m_pos < m_text.size() && m_text[m_pos] == ']')
            {
                ++m_pos;
                out = std::move(array);
                return true;
            }

            while (true)
            {
                SkipWhitespace();

                Json value;

                if (!ParseValue(value, depth + 1))
                {
                    return false;
                }

                array.Push(std::move(value));

                SkipWhitespace();

                if (m_pos >= m_text.size())
                {
                    m_message = "数组没有闭合";
                    return false;
                }

                if (m_text[m_pos] == ',')
                {
                    ++m_pos;
                    continue;
                }

                if (m_text[m_pos] == ']')
                {
                    ++m_pos;
                    out = std::move(array);
                    return true;
                }

                m_message = "数组元素之间缺少逗号或右方括号";
                return false;
            }
        }

        bool ParseString(std::string& out)
        {
            ++m_pos; // '"'
            out.clear();

            while (m_pos < m_text.size())
            {
                const unsigned char ch =
                    static_cast<unsigned char>(m_text[m_pos++]);

                if (ch == '"')
                {
                    return true;
                }

                if (ch < 0x20)
                {
                    m_message = "字符串中出现未转义的控制字符";
                    return false;
                }

                if (ch != '\\')
                {
                    out.push_back(static_cast<char>(ch));
                    continue;
                }

                if (m_pos >= m_text.size())
                {
                    m_message = "转义字符不完整";
                    return false;
                }

                const char escape = m_text[m_pos++];

                switch (escape)
                {
                case '"':  out.push_back('"');  break;
                case '\\': out.push_back('\\'); break;
                case '/':  out.push_back('/');  break;
                case 'b':  out.push_back('\b'); break;
                case 'f':  out.push_back('\f'); break;
                case 'n':  out.push_back('\n'); break;
                case 'r':  out.push_back('\r'); break;
                case 't':  out.push_back('\t'); break;

                case 'u':
                {
                    unsigned int codePoint = 0;

                    if (!ReadHex4(codePoint))
                    {
                        return false;
                    }

                    // 代理对：高代理 + 低代理
                    if (codePoint >= 0xD800 && codePoint <= 0xDBFF)
                    {
                        if (m_pos + 1 >= m_text.size() ||
                            m_text[m_pos] != '\\' ||
                            m_text[m_pos + 1] != 'u')
                        {
                            m_message = "高位代理之后缺少低位代理";
                            return false;
                        }

                        m_pos += 2;

                        unsigned int low = 0;

                        if (!ReadHex4(low))
                        {
                            return false;
                        }

                        if (low < 0xDC00 || low > 0xDFFF)
                        {
                            m_message = "低位代理取值非法";
                            return false;
                        }

                        codePoint = 0x10000 +
                            ((codePoint - 0xD800) << 10) +
                            (low - 0xDC00);
                    }

                    AppendUtf8(out, codePoint);
                    break;
                }

                default:
                    m_message = "未知的转义字符";
                    return false;
                }
            }

            m_message = "字符串没有闭合";
            return false;
        }

        bool ReadHex4(unsigned int& value)
        {
            if (m_pos + 4 > m_text.size())
            {
                m_message = "\\u 转义缺少 4 位十六进制数";
                return false;
            }

            value = 0;

            for (int i = 0; i < 4; ++i)
            {
                const char ch = m_text[m_pos++];
                value <<= 4;

                if (ch >= '0' && ch <= '9')
                {
                    value |= static_cast<unsigned int>(ch - '0');
                }
                else if (ch >= 'a' && ch <= 'f')
                {
                    value |= static_cast<unsigned int>(ch - 'a' + 10);
                }
                else if (ch >= 'A' && ch <= 'F')
                {
                    value |= static_cast<unsigned int>(ch - 'A' + 10);
                }
                else
                {
                    m_message = "\\u 转义中出现非十六进制字符";
                    return false;
                }
            }

            return true;
        }

        static void AppendUtf8(std::string& out, unsigned int codePoint)
        {
            if (codePoint < 0x80)
            {
                out.push_back(static_cast<char>(codePoint));
            }
            else if (codePoint < 0x800)
            {
                out.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
                out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
            }
            else if (codePoint < 0x10000)
            {
                out.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
                out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
            }
            else
            {
                out.push_back(static_cast<char>(0xF0 | (codePoint >> 18)));
                out.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
            }
        }

        bool ParseNumber(Json& out)
        {
            const std::size_t start = m_pos;

            if (m_text[m_pos] == '-')
            {
                ++m_pos;
            }

            if (m_pos >= m_text.size() ||
                m_text[m_pos] < '0' || m_text[m_pos] > '9')
            {
                m_message = "这里需要一个数字";
                return false;
            }

            const char* begin = m_text.data() + start;
            const char* end = m_text.data() + m_text.size();

            double value = 0.0;
            const auto result = std::from_chars(begin, end, value);

            if (result.ec != std::errc())
            {
                m_pos = start;
                m_message = "数字格式非法";
                return false;
            }

            m_pos = static_cast<std::size_t>(result.ptr - m_text.data());

            // JSON 不允许数字后面直接跟字母等字符（例如 1abc）
            if (m_pos < m_text.size())
            {
                const char next = m_text[m_pos];

                if ((next >= '0' && next <= '9') ||
                    (next >= 'a' && next <= 'z') ||
                    (next >= 'A' && next <= 'Z') ||
                    next == '.')
                {
                    m_message = "数字格式非法";
                    return false;
                }
            }

            out = Json(value);
            return true;
        }

        bool MatchLiteral(const char* literal)
        {
            const std::size_t length = std::strlen(literal);

            if (m_text.compare(m_pos, length, literal) != 0)
            {
                m_message = "无法识别的字面量";
                return false;
            }

            m_pos += length;
            return true;
        }

        void SkipWhitespace()
        {
            while (m_pos < m_text.size())
            {
                const char ch = m_text[m_pos];

                if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
                {
                    ++m_pos;
                }
                else
                {
                    break;
                }
            }
        }

        std::string MakeError(const std::string& message) const
        {
            int line = 1;
            int column = 1;

            const std::size_t limit = m_pos < m_text.size()
                ? m_pos : m_text.size();

            for (std::size_t i = 0; i < limit; ++i)
            {
                if (m_text[i] == '\n')
                {
                    ++line;
                    column = 1;
                }
                else
                {
                    ++column;
                }
            }

            return "第 " + std::to_string(line) + " 行第 " +
                std::to_string(column) + " 列：" + message;
        }

        const std::string& m_text;
        std::size_t m_pos = 0;
        std::string m_message;
    };
}

bool Json::Parse(const std::string& text, Json& out, std::string& error)
{
    Parser parser(text);
    return parser.Run(out, error);
}
