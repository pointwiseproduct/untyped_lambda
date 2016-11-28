#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <locale>
#include <fstream>
#include <memory>
#include <set>
#include <map>
#include <tuple>
#include <clocale>
#include <cstdlib>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#ifdef _MSC_VER
#include <conio.h>
#endif

namespace fs = boost::filesystem;

// プログラムスイッチ．
std::set<std::string> program_switchs;

// アプリケーション全般で扱う例外クラス．
// アプリケーションからI/Oへ出力される例外は全てこれを経由する．
class app_exception : public std::runtime_error{
public:
    app_exception() : std::runtime_error("untyped lambda: unknown error."){}
    app_exception(const app_exception &other) : std::runtime_error(other){}
    app_exception(std::string str) : std::runtime_error("error: " + str){}
    virtual ~app_exception(){}
};

// ファイルが見つからない例外．
class no_file_exist : public app_exception{
public:
    no_file_exist(std::string path) : app_exception("no file exist: " + path){}
    no_file_exist(const no_file_exist &other) = default;
    ~no_file_exist() = default;
};

// ワイド文字をマルチバイト文字に変換する．
std::string wstring_to_string(std::wstring str){
    std::vector<char> v(str.size());
    std::wcstombs(&v[0], str.c_str(), str.size());
    return &v[0];
}

// ファイルの有無をチェックする．
void check_file_exist(const fs::path &path){
    if(!fs::exists(path)){
        throw no_file_exist(path.c_str());
    }
}

// ファイルを開いた際の例外．
class open_file_exception : public app_exception{
public:
    open_file_exception(std::string path, std::string msg) : app_exception("file open failed: " + path + " : " + msg){}
    open_file_exception(const open_file_exception &other) : app_exception(other){}
    ~open_file_exception() override{}
};

// ファイルを開く．
std::vector<char> open_file(const fs::path &path){
    try{
        const boost::uintmax_t size = fs::file_size(path);
        std::vector<char> r(size);
        std::ifstream ifile(path.c_str(), std::ios::binary);
        ifile.read(&r[0], size);
        return r;
    }catch(fs::filesystem_error ex){
        throw open_file_exception(path.c_str(), ex.what());
    }catch(...){
        throw app_exception();
    }
}

namespace tokenize_phase1{
    using storage_type = std::vector<char>;

    struct token_t{
        enum class kind_t{
            white,
            charactor,
            equal,
            asterisk,
            lambda,
            dot,
            lparen,
            rparen,
            end
        };

        kind_t kind;
        storage_type::const_iterator iter;
        std::size_t line, colunm;
    };

    using token_seq_type = std::vector<token_t>;

    token_seq_type tokenize(const storage_type &vec){
        token_seq_type r;
        std::size_t line = 0, colunm = 0;
        for(auto iter = vec.begin(); iter != vec.end(); ++iter){
            char c = *iter;
            switch(c){
            case '/':
            case '\\':
                {
                    token_t t;
                    t.kind = token_t::kind_t::lambda;
                    t.iter = iter;
                    t.line = line;
                    t.colunm = colunm;
                    colunm += 1;
                    r.push_back(t);
                }
                break;

            case '=':
                {
                    token_t t;
                    t.kind = token_t::kind_t::equal;
                    t.iter = iter;
                    t.line = line;
                    t.colunm = colunm;
                    colunm++;
                    r.push_back(t);
                }
                break;
            
            case '*':
                {
                    token_t t;
                    t.kind = token_t::kind_t::asterisk;
                    t.iter = iter;
                    t.line = line;
                    t.colunm = colunm;
                    colunm++;
                    r.push_back(t);
                }
                break;

            case '.':
                {
                    token_t t;
                    t.kind = token_t::kind_t::dot;
                    t.iter = iter;
                    t.line = line;
                    t.colunm = colunm;
                    colunm++;
                    r.push_back(t);
                }
                break;

            case '(':
                {
                    token_t t;
                    t.kind = token_t::kind_t::lparen;
                    t.iter = iter;
                    t.line = line;
                    t.colunm = colunm;
                    colunm++;
                    r.push_back(t);
                }
                break;

            case ')':
                {
                    token_t t;
                    t.kind = token_t::kind_t::rparen;
                    t.iter = iter;
                    t.line = line;
                    t.colunm = colunm;
                    colunm++;
                    r.push_back(t);
                }
                break;

            case ' ':
                {
                    token_t t;
                    t.kind = token_t::kind_t::white;
                    t.iter = iter;
                    t.line = line;
                    t.colunm = colunm;
                    r.push_back(t);
                }
                colunm++;
                break;

            case '\t':
                {
                    token_t t;
                    t.kind = token_t::kind_t::white;
                    t.iter = iter;
                    t.line = line;
                    t.colunm = colunm;
                    r.push_back(t);
                }
                colunm += 4;
                break;

            case '\n':
            case '\r':
                {
                    token_t t;
                    t.kind = token_t::kind_t::white;
                    t.iter = iter;
                    t.line = line;
                    t.colunm = colunm;
                    r.push_back(t);
                }
                if(c == '\n'){
                    ++line;
                }
                colunm = 0;
                break;

            default:
                {
                    token_t t;
                    t.kind = token_t::kind_t::charactor;
                    t.iter = iter;
                    t.line = line;
                    t.colunm = colunm;
                    colunm++;
                    r.push_back(t);
                }
                break;
            }
        }
        {
            token_t t;
            t.kind = token_t::kind_t::end;
            t.iter = vec.end();
            t.line = line;
            t.colunm = colunm + 1;
            r.push_back(t);
        }
        return std::move(r);
    }
}

namespace tokenize_phase2{
    using storage_type = std::vector<char>;

    struct token_t{
        enum class kind_t{
            variable,
            lambda,
            equal,
            dot,
            lparen,
            rparen,
            end
        };

        kind_t kind;

        storage_type::const_iterator beg, end;
        std::size_t line, colunm;
    };

    using token_seq_type = std::vector<token_t>;

    class unexpected_eof : public app_exception{
    public:
        unexpected_eof() : app_exception("detected unexpected eof."){}
        unexpected_eof(const unexpected_eof&) = default;
        ~unexpected_eof() = default;
    };

    token_seq_type tokenize(const tokenize_phase1::token_seq_type &vec){
        token_seq_type r;
        for(auto iter = vec.begin(); iter != vec.end(); ++iter){
            auto t = *iter;
            if(t.kind == tokenize_phase1::token_t::kind_t::lparen){
                auto lookahead = *(iter + 1);
                if(lookahead.kind == tokenize_phase1::token_t::kind_t::asterisk){
                    iter += 2;
                    for(; ; ++iter){
                        t = *iter;
                        if(t.kind == tokenize_phase1::token_t::kind_t::end){
                            throw unexpected_eof();
                        }
                        if(
                            t.kind == tokenize_phase1::token_t::kind_t::asterisk &&
                            (iter + 1)->kind == tokenize_phase1::token_t::kind_t::rparen
                        ){
                            ++iter;
                            break;
                        }
                    }
                    continue;
                }else{
                    token_t u;
                    u.beg = t.iter;
                    u.end = t.iter + 1;
                    u.line = t.line;
                    u.colunm = t.colunm;
                    u.kind = token_t::kind_t::lparen;
                    r.push_back(u);
                    continue;
                }
            }else if(t.kind != tokenize_phase1::token_t::kind_t::charactor){
                switch(t.kind){
                case tokenize_phase1::token_t::kind_t::lambda:
                    {
                        token_t u;
                        u.beg = t.iter;
                        u.end = t.iter + 1;
                        u.line = t.line;
                        u.colunm = t.colunm;
                        u.kind = token_t::kind_t::lambda;
                        r.push_back(u);
                    }
                    break;

                case tokenize_phase1::token_t::kind_t::equal:
                    {
                        token_t u;
                        u.beg = t.iter;
                        u.end = t.iter + 1;
                        u.line = t.line;
                        u.colunm = t.colunm;
                        u.kind = token_t::kind_t::equal;
                        r.push_back(u);
                    }
                    break;

                case tokenize_phase1::token_t::kind_t::dot:
                    {
                        token_t u;
                        u.beg = t.iter;
                        u.end = t.iter + 1;
                        u.line = t.line;
                        u.colunm = t.colunm;
                        u.kind = token_t::kind_t::dot;
                        r.push_back(u);
                    }
                    break;

                case tokenize_phase1::token_t::kind_t::lparen:
                    {
                        token_t u;
                        u.beg = t.iter;
                        u.end = t.iter + 1;
                        u.line = t.line;
                        u.colunm = t.colunm;
                        u.kind = token_t::kind_t::lparen;
                        r.push_back(u);
                    }
                    break;

                case tokenize_phase1::token_t::kind_t::rparen:
                    {
                        token_t u;
                        u.beg = t.iter;
                        u.end = t.iter + 1;
                        u.line = t.line;
                        u.colunm = t.colunm;
                        u.kind = token_t::kind_t::rparen;
                        r.push_back(u);
                    }
                    break;

                case tokenize_phase1::token_t::kind_t::end:
                    {
                        token_t u;
                        u.beg = t.iter;
                        u.end = t.iter;
                        u.line = t.line;
                        u.colunm = t.colunm;
                        u.kind = token_t::kind_t::end;
                        r.push_back(u);
                    }
                    break;
                }
                continue;
            }else if(t.kind == tokenize_phase1::token_t::kind_t::charactor){
                token_t u;
                u.beg = t.iter;
                u.line = t.line;
                u.colunm = t.colunm;
                u.kind = token_t::kind_t::variable;
                while(iter->kind == tokenize_phase1::token_t::kind_t::charactor){
                    ++iter;
                }
                u.end = iter->iter;
                --iter;
                r.push_back(u);
                continue;
            }
        }
        return r;
    }
}

// λ計算の内部表現構造．
namespace internal_data{
    struct expr{
        enum class kind{
            variable,
            sequence,
            lambda
        };

        using variable_map = std::map<std::string, const expr*>;
        using expr_lookup_table = std::map<std::string, std::unique_ptr<expr>>;

        virtual kind get_kind() const = 0;
        virtual expr *copy() const = 0;
        virtual expr *replace(const variable_map&, const expr_lookup_table&, bool &mod) const = 0;
        virtual std::string to_str() const = 0;
        virtual bool equal(const expr*) const = 0;
        virtual ~expr() = default;
    };

    struct variable : public expr{
        kind get_kind() const override{
            return kind::variable;
        }

        expr *copy() const override{
            variable *r = new variable;
            r->str = str;
            return r;
        }

        expr *replace(const variable_map &map, const expr_lookup_table &global_map, bool &mod) const override{
            auto iter = map.find(str);
            if(iter != map.end()){
                mod = true;
                return iter->second->copy();
            }else{
                auto jter = global_map.find(str);
                if(jter != global_map.end()){
                    mod = true;
                    return jter->second->copy();
                }else{
                    return copy();
                }
            }
        }

        std::string to_str() const override{
            return str;
        }

        virtual bool equal(const expr *other) const{
            return
                other->get_kind() == expr::kind::variable &&
                static_cast<const variable*>(other)->str == str;
        }

        std::string str;
    };

    struct sequence : public expr{
        kind get_kind() const override{
            return kind::sequence;
        }

        expr *copy() const override{
            sequence *r = new sequence;
            for(auto iter = vec.begin(); iter != vec.end(); ++iter){
                r->push_back(std::move(std::unique_ptr<expr>((*iter)->copy())));
            }
            return r;
        }

        expr *replace(const variable_map &map, const expr_lookup_table &global_map, bool &mod) const override{
            sequence *r = new sequence;
            for(auto iter = vec.begin(); iter != vec.end(); ++iter){
                r->push_back(std::move(std::unique_ptr<expr>((*iter)->replace(map, global_map, mod))));
            }
            return r;
        }

        std::string to_str() const override{
            std::string r;
            std::size_t count = 0;
            for(auto &i : vec){
                bool nest = i->get_kind() == kind::sequence || i->get_kind() == kind::lambda;
                if(nest){
                    r += "(";
                }
                r += i->to_str();
                if(nest){
                    r += ")";
                }
                if(count < vec.size() - 1){
                    r += " ";
                }
                ++count;
            }
            return std::move(r);
        }

        virtual bool equal(const expr *other) const{
            if(other->get_kind() != expr::kind::sequence){
                return false;
            }
            const sequence *other_seq = static_cast<const sequence*>(other);
            auto iter = vec.begin();
            for(auto &i : other_seq->vec){
                if(!i->equal(&*i)){
                    return false;
                }
                ++iter;
            }
            return true;
        }

        std::vector<std::unique_ptr<expr>> vec;
        void push_back(std::unique_ptr<expr> e){
            vec.push_back(std::move(e));
        }
    };

    struct lambda : public expr{
        lambda() : seq(new sequence){}

        kind get_kind() const{
            return kind::lambda;
        }

        expr *copy() const{
            lambda *r = new lambda;
            for(auto iter = variable_seq.begin(); iter != variable_seq.end(); ++iter){
                variable v;
                v.str = iter->str;
                r->variable_seq.push_back(v);
            }
            for(auto iter = get_seq()->vec.begin(); iter != get_seq()->vec.end(); ++iter){
                r->get_seq()->push_back(std::move(std::unique_ptr<expr>((*iter)->copy())));
            }
            return r;
        }

        variable_map make_dropped_map(const variable_map &map) const{
            variable_map dropped_map = map;
            for(const variable &var_seq_iter : variable_seq){
                auto iter = dropped_map.find(var_seq_iter.str);
                if(iter != dropped_map.end()){
                    dropped_map.erase(iter);
                }
            }
            return std::move(dropped_map);
        }

        expr *replace(const variable_map &map, const expr_lookup_table &global_map, bool &mod) const{
            variable_map dropped_map = make_dropped_map(map);
            lambda *lam = new lambda;
            lam->variable_seq = variable_seq;
            std::unique_ptr<expr> seq_prime(seq->copy());
            lam->seq.swap(seq_prime);
            std::unique_ptr<expr> f(seq->replace(dropped_map, global_map, mod));
            f.swap(lam->seq);
            return lam;
        }

        std::string to_str() const{
            std::string r;
            r += "/";
            std::size_t count = 0;
            for(auto &i : variable_seq){
                r += i.to_str();
                if(count < variable_seq.size() - 1){
                    r += " ";
                }
                ++count;
            }
            r += ". ";
            r += seq->to_str();
            return std::move(r);
        }

        bool equal(const expr *other) const{
            if(other->get_kind() != expr::kind::lambda){
                return false;
            }
            const lambda *other_seq = static_cast<const lambda*>(other);
            if(variable_seq.size() != other_seq->variable_seq.size()){
                return false;
            }
            {
                auto iter = variable_seq.begin();
                for(auto &i : other_seq->variable_seq){
                    if(!iter->equal(&i)){
                        return false;
                    }
                    ++iter;
                }
            }
            const sequence *s = static_cast<const sequence*>(seq.get());
            const sequence *other_s = static_cast<const sequence*>(other_seq->seq.get());
            auto iter = s->vec.begin();
            for(auto &i : other_s->vec){
                if(!i->equal(&*i)){
                    return false;
                }
                ++iter;
            }
            return true;
        }

        std::vector<variable> variable_seq;
        mutable std::unique_ptr<expr> seq;

        sequence *get_seq() const{
            return static_cast<sequence*>(seq.get());
        }
    };

    expr::expr_lookup_table assignment_table;

    struct step_out{};

    bool eval(std::unique_ptr<expr> &e, int &nest_level, bool step = false){
        bool mod = false;
        if(e->get_kind() == expr::kind::sequence){
            while(true){
                sequence *seq = static_cast<sequence*>(e.get());
                expr::kind kind = seq->vec[0]->get_kind();
                if(kind == expr::kind::lambda){
                    lambda &lam = static_cast<lambda&>(*static_cast<lambda*>(seq->vec[0].get()));
                    std::unique_ptr<expr> &lam_expr = seq->vec[0];
                    std::size_t s = seq->vec.size() - 1;
                    expr::variable_map map;
                    if(lam.variable_seq.size() <= s){
                        for(std::size_t i = 0; i < lam.variable_seq.size(); ++i){
                            map.insert(std::make_pair(lam.variable_seq[i].str, seq->vec[i + 1].get()));
                        }
                        {
                            expr *f = lam.seq->replace(map, assignment_table, mod);
                            seq->vec.erase(seq->vec.begin(), seq->vec.begin() + s + 1);
                            seq->vec.insert(seq->vec.begin(), std::move(std::unique_ptr<expr>(f)));
                        }
                        if(step && mod){
                            throw step_out();
                        }
                        if(seq->vec.size() > 1){
                            continue;
                        }else{
                            std::unique_ptr<expr> f(std::move(*seq->vec.begin()));
                            *seq->vec.begin() = nullptr;
                            e.swap(f);
                            if(eval(e, nest_level, step)){
                                if(step){
                                    throw step_out();
                                }
                                mod = false;
                            }
                            break;
                        }
                    }else{
                        for(std::size_t i = 0; i < s; ++i){
                            map.insert(std::make_pair(lam.variable_seq[i].str, seq->vec[i + 1].get()));
                        }
                        std::unique_ptr<expr> f(lam.seq->replace(map, assignment_table, mod));
                        if(f->get_kind() == expr::kind::sequence){
                            lam.seq.swap(f);
                        }else{
                            lam.get_seq()->vec.clear();
                            lam.get_seq()->vec.push_back(std::move(f));
                        }
                        lam.variable_seq.erase(lam.variable_seq.begin(), lam.variable_seq.begin() + s);
                        e.swap(lam_expr);
                        if(step){
                            throw step_out();
                        }
                        break;
                    }
                }else if(kind == expr::kind::sequence){
                    std::unique_ptr<expr> &f(seq->vec[0]);
                    ++nest_level;
                    if(eval(f, nest_level, step)){
                        if(step){
                            throw step_out();
                        }
                    }
                    --nest_level;
                    std::unique_ptr<expr> tmp(f->copy());
                    seq->vec[0].swap(tmp);
                    if(seq->vec[0]->get_kind() == expr::kind::lambda){
                        continue;
                    }else{
                        break;
                    }
                }else{
                    if(seq->vec.size() == 1){
                        std::unique_ptr<expr> f(nullptr);
                        seq->vec[0].swap(f);
                        e.swap(f);
                    }
                    break;
                }
            }
            if(e->get_kind() == expr::kind::sequence){
                sequence *seq = static_cast<sequence*>(e.get());
                if(seq->vec.size() > 1){
                    for(auto iter = seq->vec.begin(); iter != seq->vec.end(); ++iter){
                        ++nest_level;
                        if(eval(*iter, nest_level, step)){
                            if(step){
                                throw step_out();
                            }
                        }
                        --nest_level;
                    }
                }else{
                    e.swap(*seq->vec.begin());
                }
            }else if(e->get_kind() == expr::kind::lambda){
                lambda *lam = static_cast<lambda*>(e.get());
                ++nest_level;
                if(eval(lam->seq, nest_level, step)){
                    if(step){
                        throw step_out();
                    }
                }
                --nest_level;
                if(lam->seq->get_kind() != expr::kind::sequence){
                    std::unique_ptr<expr> seq(new sequence);
                    static_cast<sequence*>(seq.get())->vec.push_back(std::move(static_cast<lambda*>(e.get())->seq));
                    lam->seq.swap(seq);
                }else{
                    sequence *seq = static_cast<sequence*>(lam->seq.get());
                    for(auto iter = seq->vec.begin(); iter != seq->vec.end(); ++iter){
                        ++nest_level;
                        if(eval(*iter, nest_level, step)){
                            if(step){
                                throw step_out();
                            }
                        }
                        --nest_level;
                    }
                }
            }
        }else if(e->get_kind() == expr::kind::lambda){
            lambda *lam = static_cast<lambda*>(e.get());
            ++nest_level;
            if(eval(lam->seq, nest_level, step)){
                if(step){
                    throw step_out();
                }
            }
            --nest_level;
            if(lam->seq->get_kind() != expr::kind::sequence){
                std::unique_ptr<expr> seq(new sequence);
                static_cast<sequence*>(seq.get())->vec.push_back(std::move(static_cast<lambda*>(e.get())->seq));
                lam->seq.swap(seq);
            }
        }
        return mod;
    }

    std::vector<std::unique_ptr<expr>> lines;
}

namespace parsing_phase{
    using token_seq_type = tokenize_phase2::token_seq_type;
    using token_t = tokenize_phase2::token_t;
    using kind_t = token_t::kind_t;

    token_seq_type::const_iterator expr(std::unique_ptr<internal_data::expr>&, token_seq_type::const_iterator);
    token_seq_type::const_iterator expr_no_sequence(std::unique_ptr<internal_data::expr>&, token_seq_type::const_iterator);

    // パージングエラー．
    class parsing_error : public app_exception{
    public:
        parsing_error(std::size_t line) : app_exception("parsing error: " + std::to_string(line + 1)){}
        parsing_error(const parsing_error&) = default;
        ~parsing_error() = default;
    };

    // 代入式．
    token_seq_type::const_iterator assignment(std::unique_ptr<internal_data::expr> &e, token_seq_type::const_iterator first){
        token_seq_type::const_iterator iter = first;
        std::string name;
        if(iter->kind == kind_t::variable){
            name = std::string(iter->beg, iter->end);
        }else{
            return first;
        }
        ++iter;
        if(iter->kind != kind_t::equal){
            return first;
        }
        ++iter;
        std::unique_ptr<internal_data::expr> f;
        token_seq_type::const_iterator result(expr(f, iter));
        if(result == iter){
            return std::move(result);
        }
        e.swap(f);
        internal_data::assignment_table.insert(std::make_pair(name, std::move(std::unique_ptr<internal_data::expr>(e->copy()))));
        return result;
    }

    // 行．
    token_seq_type::const_iterator line(token_seq_type::const_iterator first){
        std::unique_ptr<internal_data::expr> e;
        auto result = assignment(e, first);
        if(result == first){
            result = expr(e, first);
            if(result == first){
                throw parsing_error(first->line);
            }
            internal_data::lines.push_back(std::move(e));
        }
        return result;
    }

    // sequence．
    token_seq_type::const_iterator sequence(token_seq_type::const_iterator first, std::unique_ptr<internal_data::expr> &seq){
        token_seq_type::const_iterator iter = first;
        token_seq_type::const_iterator result = expr(seq, iter);
        if(result != first && seq->get_kind() != internal_data::expr::kind::sequence){
            internal_data::sequence *wrapper_seq = new internal_data::sequence;
            std::unique_ptr<internal_data::expr> wrapper(wrapper_seq);
            wrapper_seq->push_back(std::move(seq));
            seq.swap(wrapper);
        }
        return result;
    }

    // lambda．
    token_seq_type::const_iterator lambda(token_seq_type::const_iterator first, internal_data::lambda &lam){
        token_seq_type::const_iterator iter = first;
        if(iter->kind != kind_t::lambda){
            return first;
        }
        ++iter;
        while(iter->kind == kind_t::variable){
            internal_data::variable v;
            v.str = std::string(iter->beg, iter->end);
            lam.variable_seq.push_back(v);
            ++iter;
        }
        if(iter->kind != kind_t::dot){
            return first;
        }
        ++iter;
        token_seq_type::const_iterator result = sequence(iter, lam.seq);
        if(result == iter){
            // sequenceのパース失敗．
            return first;
        }
        return result;
    }

    // variable．
    token_seq_type::const_iterator variable(token_seq_type::const_iterator first, internal_data::variable &var){
        token_seq_type::const_iterator iter = first;
        if(iter->kind == kind_t::variable){
            var.str = std::string(iter->beg, iter->end);
            return ++iter;
        }else{
            return first;
        }
    }

    // 抽象式．
    // シーケンスを含まない．
    token_seq_type::const_iterator expr_no_sequence(std::unique_ptr<internal_data::expr> &e, token_seq_type::const_iterator first){
        token_seq_type::const_iterator iter = first;
        token_seq_type::const_iterator r = first;
        if(iter->kind == kind_t::lambda){
            internal_data::lambda *ptr = new internal_data::lambda();
            iter = lambda(iter, *ptr);
            e.reset(ptr);
            r = iter;
        }else if(iter->kind == kind_t::variable){
            internal_data::variable *ptr = new internal_data::variable();
            iter = variable(iter, *ptr);
            e.reset(ptr);
            r = iter;
        }else if(iter->kind == kind_t::lparen){
            ++iter;
            std::unique_ptr<internal_data::expr> f;
            token_seq_type::const_iterator s = expr(f, iter);
            if(s != first){
                iter = s;
                if(iter->kind == kind_t::rparen){
                    ++s;
                    e.swap(f);
                    r = s;
                }else{
                    throw parsing_error(first->line);
                }
            }
        }
        return r;
    }

    // 抽象式．
    token_seq_type::const_iterator expr(std::unique_ptr<internal_data::expr> &e, token_seq_type::const_iterator first){
        token_seq_type::const_iterator r;
        token_seq_type::const_iterator iter = first;
        internal_data::sequence *seq = nullptr;
        std::unique_ptr<internal_data::expr> seq_expr(nullptr);
        while(true){
            auto case_seq_new = [&](){
                if(!seq){
                    seq_expr.reset(new internal_data::sequence());
                    seq = static_cast<internal_data::sequence*>(seq_expr.get());
                }
                seq->push_back(std::move(e));
            };

            if(iter->kind == kind_t::lambda){
                internal_data::lambda *ptr = new internal_data::lambda();
                iter = lambda(iter, *ptr);
                e.reset(ptr);
                r = iter;
                if(iter == first){
                    break;
                }

                if(iter->kind == kind_t::dot){
                    ++iter;
                    if(iter->kind == kind_t::end){
                        ++r;
                    }
                    if(seq){
                        seq->push_back(std::move(e));
                    }
                    break;
                }
                case_seq_new();
                if(iter->kind == kind_t::rparen){
                    break;
                }
            }else if(iter->kind == kind_t::variable){
                internal_data::variable *ptr = new internal_data::variable();
                iter = variable(iter, *ptr);
                e.reset(ptr);
                r = iter;
                if(iter == first){
                    break;
                }

                if(iter->kind == kind_t::dot){
                    ++iter;
                    if(iter->kind == kind_t::end){
                        ++r;
                    }
                    if(seq){
                        seq->push_back(std::move(e));
                    }
                    break;
                }
                case_seq_new();
                if(iter->kind == kind_t::rparen){
                    break;
                }
            }else if(iter->kind == kind_t::lparen){
                if(iter->kind == kind_t::lparen){
                    ++iter;
                    std::unique_ptr<internal_data::expr> f;
                    token_seq_type::const_iterator s = expr(f, iter);
                    if(s != first){
                        iter = s;
                        if(iter->kind == kind_t::rparen){
                            ++s;
                            ++iter;
                            e.swap(f);
                            r = s;
                        }else{
                            throw parsing_error(first->line);
                        }
                    }
                }

                if(iter->kind == kind_t::dot){
                    ++iter;
                    if(iter->kind == kind_t::end){
                        ++r;
                    }
                    if(seq){
                        seq->push_back(std::move(e));
                    }
                    break;
                }
                case_seq_new();
                if(iter->kind == kind_t::rparen){
                    break;
                }
            }else if(iter->kind != kind_t::dot){
                return first;
            }
        }
        if(seq){
            if(seq->vec.size() != 1){
                e.swap(seq_expr);
            }else{
                e.swap(seq->vec[0]);
            }
        }
        return r;
    }

    // 複数行をパースする．
    bool lines(token_seq_type::const_iterator first){
        token_seq_type::const_iterator iter = first;
        token_seq_type::const_iterator jter = iter;
        for(; (jter = line(iter)) != iter && jter->kind != kind_t::end; iter = jter){
            if(jter->kind == kind_t::dot){
                ++jter;
            }
        }
        if(jter->kind == kind_t::end){
            return true;
        }else{
            return false;
        }
    }
}

class parsing_failed : public app_exception{
public:
    parsing_failed() : app_exception("parsing failed."){}
    parsing_failed(const parsing_failed&) = default;
    ~parsing_failed() override = default;
};

int waiting(){
#ifdef _MSC_VER
    return getch();
#else
    return getchar();
#endif
}

void launch_interpreter(){
    while(true){
        internal_data::lines.clear();
        std::string line;
        std::getline(std::cin, line);
        if(line == "exit" || line == "quit"){
            break;
        }

        try{
            std::vector<char> line_raw(line.begin(), line.end());
            auto b = tokenize_phase2::tokenize(tokenize_phase1::tokenize(line_raw));
            if(!parsing_phase::lines(b.begin())){
                throw parsing_failed();
            }
            for(auto &i : internal_data::lines){
                bool mod;
                std::unique_ptr<internal_data::expr> q(i->replace(internal_data::expr::variable_map(), internal_data::assignment_table, mod));

                while(true){
                    try{
                        int nest_level = 0;
                        internal_data::eval(q, nest_level, true);
                        std::cout << " = " << q->to_str() << "." << std::endl;
                    }catch(internal_data::step_out){
                        std::cout << " = " << q->to_str() << "." << std::endl;
                        if(waiting() == 'c'){
                            break;
                        }
                        continue;
                    }
                    break;
                }
            }
        }catch(app_exception e){
            std::cerr << e.what() << std::endl;
        }
    }
}

int main(int argc, char *argv[]){
    if(argc <= 1){
        launch_interpreter();
        return 0;
    }

    program_switchs.insert("-b");
    for(int i = 2; i < argc; ++i){
        std::string str(argv[i]);
        if(str == "-o"){
            auto iter = program_switchs.find("-b");
            if(iter != program_switchs.end()){
                program_switchs.erase(iter);
            }
        }else if(str == "-b"){
            auto iter = program_switchs.find("-o");
            if(iter != program_switchs.end()){
                program_switchs.erase(iter);
            }
        }
        program_switchs.insert(str);
    }

    if(program_switchs.find("-h") != program_switchs.end() || program_switchs.find("--help") != program_switchs.end()){
        std::cout << "untyped lambda calcus ver 1.0.0" << std::endl;
        std::cout << "usage:" << std::endl;
        std::cout << "  untyped_lambda ifile_path.txt {option}" << std::endl;
        std::cout << std::endl;
        std::cout << "option:" << std::endl;
        // ヘルプメッセージを表示する．
        std::cout << "  --help:" << std::endl;
        std::cout << "      -h: show this message." << std::endl;
        // 各式の評価前の値を表示する．
        std::cout << "      -b: show before evaluation of fomulas. [default]" << std::endl;
        // 各式の評価結果のみ表示する．
        std::cout << "      -o: show only evaluation results." << std::endl;
        // 式の評価ごとに一時停止する．
        std::cout << "      -s: step evaluation." << std::endl << std::endl;

        return 0;
    }

    try{
        const fs::path ifile_path(argv[1]);
        check_file_exist(ifile_path);
        std::vector<char> str = open_file(ifile_path);
        auto a = tokenize_phase1::tokenize(str);
        auto b = tokenize_phase2::tokenize(a);
        if(!parsing_phase::lines(b.begin())){
            throw parsing_failed();
        }

        bool program_swtich_s = program_switchs.find("-s") != program_switchs.end();
        bool program_swtich_b = program_switchs.find("-b") != program_switchs.end();
        for(auto &i : internal_data::lines){
            bool mod;
            std::unique_ptr<internal_data::expr> q(i->replace(internal_data::expr::variable_map(), internal_data::assignment_table, mod));
            if(program_swtich_b || program_swtich_s){
                std::cout << i->to_str() << std::endl;
                if(program_swtich_s){
                    waiting();
                }
            }
            if(program_swtich_s){
                while(true){
                    try{
                        int nest_level = 0;
                        internal_data::eval(q, nest_level, true);
                    }catch(internal_data::step_out){
                        std::cout << q->to_str() << "." << std::endl;
                        waiting();
                        continue;
                    }
                    break;
                }
            }else{
                int nest_level = 0;
                internal_data::eval(q, nest_level);
            }
            if(program_swtich_b || program_swtich_s){
                std::cout << "-> ";
            }
            std::cout << q->to_str() << "." << std::endl;
        }
    }catch(app_exception e){
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
