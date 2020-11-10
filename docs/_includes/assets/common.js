$(document).ready(function() {
    function initialize(name) {
        let link = $(".wy-menu-vertical").find(`[href="${decodeURI(name)}"]`);
        if (link.length > 0) {
            $(".wy-menu-vertical .current").removeClass("current");
            link.addClass("current");
            link.closest("li.toctree-l1").parent().addClass("current");
            link.closest("li.toctree-l1").addClass("current");
            link.closest("li.toctree-l2").addClass("current");
            link.closest("li.toctree-l3").addClass("current");
            link.closest("li.toctree-l4").addClass("current");
            link.closest("li.toctree-l5").addClass("current");
        }
    }

    function toggleCurrent(link) {
        let closest = link.closest("li");
        closest.siblings("li.current").removeClass("current");
        closest.siblings().find("li.current").removeClass("current");
        closest.find("> ul li.current").removeClass("current");
        closest.toggleClass("current");
    }

    function set(name, value) {
        return localStorage.setItem(name, value);
    }

    function get(name) {
        return localStorage.getItem(name) || false;
    }

    function restore() {
        let scroll = get("scroll");
        let scrollTime = get("scrollTime");
        let scrollHost = get("scrollHost");

        if (scroll && scrollTime && scrollHost) {
            if (scrollHost == location.host && (Date.now() - scrollTime < 6e5)) {
                $(".wy-side-scroll").scrollTop(scroll);
            }
        }
        $(".wy-side-scroll").scroll(function() {
            set("scroll", this.scrollTop);
            set("scrollTime", Date.now());
            set("scrollHost", location.host);
        });
    }

    function highlight() {
        let text = new URL(location.href).searchParams.get("highlight");
        let box = ".highlighted-box";

        if (text) {
            $(".section").find("*").each(function() {
                try {
                    if (this.outerHTML.match(new RegExp(text, "im"))) {
                        $(this).addClass("highlighted-box");
                    }
                } catch (e) {
                    debug(e.message);
                }
            });
            $(".section").find(box).each(function() {
                if (($(this).find(box).length > 0)) {
                    $(this).removeClass(box);
                }
            });
        }
    }

    anchors.add();
    initialize(location.pathname);
    restore();
    highlight();

    /* nested ul */
    $(".wy-menu-vertical ul").siblings("a").each(function() {
        let link = $(this);
        let expand = $('<span class="toctree-expand"></span>');

        expand.on("click", function(e) {
            e.stopPropagation();
            toggleCurrent(link);
            return false;
        });
        link.prepend(expand);
    });

    /* admonition */
    $(".admonition-title").each(function() {
        var ui = $(this).attr("ui");
        $(this).html(ui[0].toUpperCase() + ui.substr(1));
    });

    /* bind */
    $(document).on("click", '[data-toggle="wy-nav-top"]', function() {
        $('[data-toggle="wy-nav-shift"]').toggleClass("shift");
    });
    $(document).on("click", ".wy-menu-vertical .current ul li a", function() {
        $('[data-toggle="wy-nav-shift"]').removeClass("shift");
        toggleCurrent($(this));
    });
    $(window).bind("resize", function() {
        requestAnimationFrame(function() {});
    });
    $(window).bind("hashchange", () => initialize(location.hash || location.pathname));
});
