#pragma once

#include <sys/types.h>
#include <cstring>
#include <string>

#include "headers/modules/Cleaner.hpp"
#include "headers/exception.hpp"

namespace modules {
    struct Zipper;
    namespace __zipper {
/** Типы переиенных */
        enum _e_vartype {
            INVALID = -1,
            INT = 1,
            UINT,
            LONG,
            ULONG,
            FLOAT,
            DOUBLE,
            CSTRING,
            GROUP,
            INSTANCE
        };
/** Маски для функций ряда 'printf' и 'scanf' */
        extern const char *const __scan_mask[];
        extern const char *const __print_mask[];
        /** после инициализации изменить будет невозможно()
         * Хотя, изменить то можно, но - не стоит, приведёт к ошибкам */
        typedef const ssize_t zv_type_t;
        typedef const char* zv_name_t;
/** базовый тип данных (instance, group и __data<T>) */
        typedef u_int64_t zv_data_t;
/** Базовый абстрактный класс переменной(в том числе и объектов-заголовков - 'instance' и 'group')
 * '__to_string()' и '__from_string()' в случае ошибки генерируют исключения:
 * 'class exception : public std::exception {};'
 * Следует заметить что прямой вызов из этого класса также сгенерирует исключение */
        struct entry_base {
        protected:
            // Конструктор: тип имя и данные(беззнаковое целое 64 бита)
            entry_base(zv_type_t type, const char* name, const zv_data_t data);
        public:
            // Деструктор
            ~entry_base();
            /** Абстрактный метод сканирующий значение из строкового представления */
            virtual ssize_t __from_string(const char* Source, const char *mask = nullptr);
            // Абстрактный метод приводящий переменную в строковое представление
            virtual ssize_t __to_string(std::string &Dest, const char *mask = nullptr) const;
            // Логическое представление переменной ('true' если 'data != 0')
            operator bool ();
        public:
            //
            zv_type_t type;
            zv_name_t name;
            zv_data_t data;
        };

/** Шаблонный класс переменной(в том числе и Объектов-заголовков: instance, group) */
        template <typename T>
        class __base : private entry_base {
        public:
            __base(const _e_vartype type, const char *name, const T data)
                : entry_base(type, name, reinterpret_cast<const u_int64_t&>(data))
            {}
            ~__base() {}
            const T& operator * () const { return reinterpret_cast<const T&>(const_cast<const u_int64_t&>(data)); }
        private:
            virtual ssize_t __from_string(const char* src, const char *mask = nullptr) {
                const char *msk = nullptr;
                if ((nullptr != src) && (INVALID < type) && (CSTRING > type)) {
                    msk = (nullptr != mask ? mask : __scan_mask[type]);
                    return sscanf(src, msk, reinterpret_cast<T&>(data));
                }
                else if (nullptr != src) {
                    throw exception(__FILE__, __LINE__, "__base<T>::__from_string() Неизвестный тип", "Zipper");
                }
                else {
                    throw exception(__FILE__, __LINE__, "__base<T>::__from_string(Src) Src == nullptr", "Zipper");
                }
            }
            virtual ssize_t __to_string(std::string &Dest, const char* mask = nullptr) const {
                char *buff = nullptr;
                if ((INVALID < type) && (CSTRING > type)) {
                    buff = new char[256];
                    if (buff != nullptr) {
                        sprintf(buff, ((nullptr == mask) ? __print_mask[type] : mask), reinterpret_cast<const T&>(data));
                        Dest += buff;
                        delete[] buff;
                    }
                    else { throw exception(__FILE__, __LINE__, "Недостаточно памяти...", "Zipper"); }
                }
                else { throw exception(__FILE__, __LINE__, "__base<T>::__to_string() Неизвестный тип", "Zipper"); }
                return 0;
            }
        protected:
            T& operator * () { return reinterpret_cast<T&>(data); }
            void setval(const T ndata) { data = reinterpret_cast<const zv_data_t&>(ndata); }
        };
        // специализация для [T=char*]
        template <>
        class __base<char*> : public entry_base {
        protected:
            __base(const _e_vartype type, const char *name, const char* data)
                : entry_base(type, name, 0)
            {
                char* cd = reinterpret_cast<char*&>(this->data);
                cd = new char[strlen(data) + 1];
                if (nullptr != cd) {
                    strcpy(cd, data);
                }
            }
        public:
            ~__base() {
                if (0 != data) {
                    delete[] reinterpret_cast<char*&>(data);
                    data = 0;
                }
            }
            const char*& operator * () const { return *((const char**)&data); }
        protected:
            char*& operator * () { return reinterpret_cast<char*&>(data); }
            void setval(const char *ndata) { data = reinterpret_cast<const zv_data_t&>(ndata); }
        private:
            virtual ssize_t __from_string(const char* src, const char *mask = nullptr) {
                char *temp = nullptr, *&cd = reinterpret_cast<char*&>(data);
                ssize_t len = 0;
                char Q = 0;
                if ((nullptr != src) && (CSTRING == type)) {
                    switch (*src) {
                        case '`':
                        case '\'':
                        case '"':
                            Q = *src;
                            ++src;
                            while ((0 != src[len]) && ((Q != src[len]) || (src[len - 1] == '\\'))) ++len;
                            break;
                        default:
                            len = 0;
                            while ((0 != src[len]) && (' ' < src[len])) ++len;
                    }
                    if (!(strncmp(cd, src, len)));
                    else {
                        if (nullptr != (temp = new char[len + 1]));
                        else {
                            throw exception(__FILE__, __LINE__, "Недостаточно памяти...", "Zipper");
                        }
                        delete[] cd;
                        cd = temp;
                        strncpy(cd, src, len);
                        cd[len] = 0;
                    }
                    return len;
                }
                else if (nullptr != src) {
                    throw exception(__FILE__, __LINE__, "__base<char*>::__from_string() Неизвестный тип", "Zipper");
                }
                else {
                    throw exception(__FILE__, __LINE__, "__base<char*>::__from_string(Src) Src == nullptr", "Zipper");
                }
            }
            virtual ssize_t __to_string(std::string &Dest, const char *mask = nullptr) const {
                char *buff = new char[1024];
                ssize_t result = -1;
                if ((CSTRING == type) && (nullptr != buff)) {
                    if (0 != (result = sprintf(buff, __print_mask[type], name, reinterpret_cast<const char*>(data)))) {
                        Dest += buff;
                    }
                    delete[] buff;
                }
                else if (nullptr == buff) {
                    Dest += name;
                    Dest += '=';
                    Dest += reinterpret_cast<const char*>(data);
                    result = 0;
                }
                else { throw exception(__FILE__, __LINE__, "__base<char*>::__to_string() Неизвестный тип", "Zipper"); }
                return result;
            }
        };

/** Абстрактный класс объекта-заголовка: файл
 * Наследники должны реализовать методы (__from/__to)_string(),
 * в противном случае будет сгенерировано исключение!!! */
        struct instance : public entry_base {
            instance(const char *Name, const char *Path, const size_t ByteCnt);
            ~instance();
            /** Абстрактный метод сканирующий значение из строкового представления */
            virtual ssize_t __from_string(const char* Source, const char *mask = nullptr);
            // Абстрактный метод приводящий переменную в строковое представление
            virtual ssize_t __to_string(std::string &Dest, const char *mask = nullptr) const;
            zv_name_t path;
        };
/** Гибкий массив для работы
 * Использование:
 * в методах упаковки/распаковки делаем каст на эту структуру и работаем как с
 * массивом
 * Zipper.unpack(Config) {
 *      __zipper::entries &list = reinterpret_cast<__zipper::entries&>(Config);
 *      ...
 * }*/
        struct entries {
            instance inst;
            entry_base v[];
        };

/** Абстрактный класс объекта-заголовка: группа
 * Наследники должны реализовать методы (__from/__to)_string(),
 * в противном случае будет сгенерировано исключение!!! */
        struct group : public entry_base {
            group(const char *Name, const size_t ByteCnt);
            ~group();
            /** Абстрактный метод сканирующий значение из строкового представления */
            virtual ssize_t __from_string(const char* Source, const char *mask = nullptr);
            // Абстрактный метод приводящий переменную в строковое представление
            virtual ssize_t __to_string(std::string &Dest, const char *mask = nullptr) const;
        };

    };
/** Абстрактный класс сериализатора */
    class Zipper {
    protected:
        Cleaner cleaner;
        int error;
    public:
/** Обёртки */
        // Заголовок-файл
        struct instance : __zipper::instance {
            instance(const char *FileName, const char *FilePath, const size_t __SizeofInst);
            ~instance();
            operator modules::__zipper::entries & ();
        };
        // Заголовок-группа
        struct group : __zipper::group {
            group(const char *GroupName, const size_t __SizeofGroup);
        };
        // целочисленная со знаком, 32 бита
        struct zipp_int32 : public __zipper::__base<int32_t> {
            zipp_int32(const char *VarName, const int32_t Dflt);
        };
        // целочисленная без знака, 32 бита
        struct zipp_uint32 : public __zipper::__base<u_int32_t> {
            zipp_uint32(const char *VarName, const u_int32_t Dflt);
        };
        // целочисленная со знаком, 64 бита
        struct zipp_int64 : public __zipper::__base<int64_t> {
            zipp_int64(const char *VarName, const int64_t Dflt);
        };
        // целочисленная без знака, 64 бита
        struct zipp_uint64 : public __zipper::__base<u_int64_t> {
            zipp_uint64(const char *VarName, const u_int64_t Dflt);
        };
        // вещественное 32 битв
        struct zipp_float : public __zipper::__base<float> {
            zipp_float(const char *VarName, const float Dflt);
        };
        // вещественное 64 битв
        struct zipp_double : public __zipper::__base<double> {
            zipp_double(const char *VarName, const double Dflt);
        };
        // "Си"-строка
        struct zipp_cstring : public __zipper::__base<char*> {
            zipp_cstring(const char *VarName, const char *Dflt);
        };

        Zipper();
        virtual ~Zipper();
        // Упаковать
        virtual int save(__zipper::entries &Inst) = 0;
        // Распаковать
        virtual int load(__zipper::entries &Inst) = 0;
    };
};
