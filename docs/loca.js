// loca.js — полноценная локализация для сайта Ely
(function() {
    'use strict';

    // ----- СЛОВАРЬ ПЕРЕВОДОВ -----
    const translations = {
        ru: {
            // Навигация
            'nav-features': 'Возможности',
            'nav-performance': 'Производительность',
            'nav-docs': 'Документация',

            // Hero
            'hero-subtitle': 'Вы выбираете, как вам писать',
            'hero-description': 'Универсальный молодой язык с простотой, гибкостью и скоростью',
            'cta-get-started': 'Начните работу',
            'cta-github': 'Исходный код',

            // Статистика
            'stat-version-label': 'стандарт',
            'stat-perf-label': 'оп/сек',
            'stat-license-label': 'лицензия',

            // Особенности (feature tiles)
            'feature-1-title': 'Безопасность памяти',
            'feature-1-desc': 'Поколенческий GC — настраиваемый, без утечек.',
            'feature-2-title': 'Делайте программы быстрыми',
            'feature-2-desc': 'Статическая типизация даёт более 20M операций/сек.',
            'feature-3-title': 'Динамический any',
            'feature-3-desc': 'Пользуйтесь удобствами динамической типизации для лёгких мест.',

            // Производительность
            'perf-heading': 'Производительность',
            'perf-subheading': 'Статика — на уровне C, динамика — удобство без жертв.',
            'perf-static-label': '(операций/сек, статический int)',
            'perf-any-label': '(операций/сек, динамический any)',

            // Таблица сравнения (заголовки)
            'th-lang': 'Язык',
            'th-typing': 'Типизация',
            'th-gc': 'GC',
            'th-cgen': 'Генерация C',
            'th-ffi': 'FFI C',
            'th-perf': 'Производительность, оп/сек',

            // Расширенные возможности
            'mod-title': 'Модульность',
            'mod-desc': 'Компиляция в .dll/.so, экспорт публичных функций.',
            'reflect-title': 'Рефлексия',
            'reflect-desc': 'typeof, fields, methods — интроспекция времени выполнения.',
            'json-title': 'File',
            'json-desc': 'Удобная работа с файлами через std модуль file',
            'ffi-title': 'C FFI',
            'ffi-desc': 'Вызов любых C‑функций, динамическая загрузка библиотек.',

            // Инструментарий
            'tooling-title': 'Инструментарий',
            'tool-compiler': 'компилятор Python → C++',
            'tool-ebt': 'ebt build - для компиляции',
            'tool-config': 'ebt project - для создания проекта',
            'tool-proj': 'elp - собственный пакетный менеджер',

            // CTA
            'cta-title': 'Начните сейчас',
            'cta-subtitle': 'Одна команда — и ваш проект готов к написанию. Открытый код, премиальная скорость',
            'cta-download': 'Скачать ElySpruce',
            'cta-docs': 'Документация',

            // Футер
            'footer-copyright': '© %year% Ely-Language. Открытый исходный код.',
            'footer-github': 'GitHub',
            'footer-telegram': 'Telegram',
            'footer-extension': 'Расширение'
        },
        en: {
            // Navigation
            'nav-features': 'Features',
            'nav-performance': 'Performance',
            'nav-docs': 'Docs',

            // Hero
            'hero-subtitle': 'You choose how to write',
            'hero-description': 'A universal young language with simplicity, flexibility, and speed',
            'cta-get-started': 'Get Started',
            'cta-github': 'Source Code',

            // Stats
            'stat-version-label': 'standard',
            'stat-perf-label': 'ops/sec',
            'stat-license-label': 'license',

            // Feature tiles
            'feature-1-title': 'Memory Safety',
            'feature-1-desc': 'Generational GC — configurable, no leaks.',
            'feature-2-title': 'Make programs fast',
            'feature-2-desc': 'Static typing delivers over 20M ops/sec.',
            'feature-3-title': 'Dynamic any',
            'feature-3-desc': 'Enjoy dynamic typing convenience for lightweight parts.',

            // Performance
            'perf-heading': 'Performance',
            'perf-subheading': 'Static — on par with C, dynamic — convenience without sacrifice.',
            'perf-static-label': '(ops/sec, static int)',
            'perf-any-label': '(ops/sec, dynamic any)',

            // Table headers
            'th-lang': 'Language',
            'th-typing': 'Typing',
            'th-gc': 'GC',
            'th-cgen': 'C Generation',
            'th-ffi': 'C FFI',
            'th-perf': 'Performance, ops/sec',

            // Extended features
            'mod-title': 'Modularity',
            'mod-desc': 'Compile to .dll/.so, export public functions.',
            'reflect-title': 'Reflection',
            'reflect-desc': 'typeof, fields, methods — runtime introspection.',
            'json-title': 'File',
            'json-desc': 'Convenient file handling via stdlib file module',
            'ffi-title': 'C FFI',
            'ffi-desc': 'Call any C functions, dynamic library loading.',

            // Tooling
            'tooling-title': 'Tooling',
            'tool-compiler': 'Python → C++ compiler',
            'tool-ebt': 'ebt build - for compilation',
            'tool-config': 'ebt project - to create a project',
            'tool-proj': 'elp - package manager',

            // CTA
            'cta-title': 'Start now',
            'cta-subtitle': 'One command and your project is ready. Open source, premium speed',
            'cta-download': 'Download ElySpruce',
            'cta-docs': 'Documentation',

            // Footer
            'footer-copyright': '© %year% Ely-Language. Open source.',
            'footer-github': 'GitHub',
            'footer-telegram': 'Telegram',
            'footer-extension': 'Extension'
        }
    };

    function getCurrentLanguage() {
        return localStorage.getItem('ely_language') || 'ru';
    }

    function setCurrentLanguage(lang) {
        localStorage.setItem('ely_language', lang);
    }

    function applyLanguage(lang) {
        const dict = translations[lang];
        if (!dict) return;

        for (const [id, text] of Object.entries(dict)) {
            const element = document.getElementById(id);
            if (element) {
                if (id === 'footer-copyright') {
                    const year = new Date().getFullYear();
                    element.innerText = text.replace('%year%', year);
                } else {
                    element.innerText = text;
                }
            }
        }

        const langOptions = document.querySelectorAll('.lang-option');
        langOptions.forEach(opt => {
            const txt = opt.innerText.trim().toLowerCase();
            if (txt === lang) {
                opt.classList.add('active');
            } else {
                opt.classList.remove('active');
            }
        });
    }

    function switchLanguage() {
        const current = getCurrentLanguage();
        const newLang = current === 'ru' ? 'en' : 'ru';
        setCurrentLanguage(newLang);
        applyLanguage(newLang);
    }

    function initLocalization() {
        const oldToggle = document.getElementById('lang-toggle');
        if (oldToggle) {
            const newToggle = oldToggle.cloneNode(true);
            oldToggle.parentNode.replaceChild(newToggle, oldToggle);
            // Добавляем свой обработчик
            newToggle.addEventListener('click', (e) => {
                e.preventDefault();
                switchLanguage();
            });
        }

        const savedLang = getCurrentLanguage();
        applyLanguage(savedLang);
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', initLocalization);
    } else {
        initLocalization();
    }
})();