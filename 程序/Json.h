// ============================================================
// Json.h —— 极简 JSON 值 / 解析器 / 序列化器
// 只依赖标准库，供 .dgate 工程文件读写使用
// ============================================================
#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class Json
{
public:
    enum class Type
    {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object
    };

    Json() = default;
    explicit Json(bool value);
    explicit Json(double value);
    explicit Json(int value);
    explicit Json(const std::string& value);
    explicit Json(const char* value);

    static Json MakeArray();
    static Json MakeObject();

    Type GetType() const { return m_type; }

    bool IsNull() const { return m_type == Type::Null; }
    bool IsBool() const { return m_type == Type::Bool; }
    bool IsNumber() const { return m_type == Type::Number; }
    bool IsString() const { return m_type == Type::String; }
    bool IsArray() const { return m_type == Type::Array; }
    bool IsObject() const { return m_type == Type::Object; }

    // -------- 数组 --------
    void Push(Json value);
    std::size_t Size() const;

    // -------- 对象 --------
    void Set(const std::string& key, Json value);
    bool Has(const std::string& key) const;

    // 键不存在时返回一个静态 Null，便于链式取值
    const Json& operator[](const std::string& key) const;

    const std::vector<Json>& Elements() const { return m_elements; }

    const std::vector<std::pair<std::string, Json>>& Members() const
    {
        return m_members;
    }

    // -------- 取值（带默认值） --------
    std::string AsString(const std::string& fallback = std::string()) const;
    double AsDouble(double fallback = 0.0) const;
    int AsInt(int fallback = 0) const;
    bool AsBool(bool fallback = false) const;

    // -------- 序列化 / 解析 --------
    std::string Dump(bool pretty = true) const;

    // 解析成功返回 true；失败时 error 带「行/列 + 原因」
    static bool Parse(const std::string& text, Json& out, std::string& error);

private:
    Type m_type = Type::Null;
    bool m_bool = false;
    double m_number = 0.0;
    std::string m_string;
    std::vector<Json> m_elements;
    std::vector<std::pair<std::string, Json>> m_members;
};
