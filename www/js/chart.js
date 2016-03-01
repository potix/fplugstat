(function($) {
    var _chartColors = ['#990033','#ff0066','#cc00cc','#6600cc','#3333ff','#003366','#009999','#00ff99',
                        '#009933','#00cc00','#ccff00','#ffcc00','#996633','#cc9966','#ff3366','#ff3399',
                        '#cc00ff','#9966ff','#3333cc','#6699cc','#669999','#339966','#33ff66','#33ff00',
                        '#999900','#ffcc66','#330000','#ffcc99','#cc0033','#ff0099','#9900cc','#330066',
                        '#0066ff','#006699','#99cccc','#006633','#00ff66','#66ff00','#cccc00','#ffcc33',
                        '#663333','#ffffff','#ff0033','#ff33cc','#990099','#6600ff','#0033ff','#3399cc',
                        '#ccffff','#336633','#ccffcc','#99ff00','#cccc33','#cc9933','#996666','#cccccc',
                        '#ff9999','#ff00cc','#cc99cc','#6633ff','#3366ff','#0099cc','#33cccc','#669966',
                        '#ccff99','#66cc00','#333300','#996600','#cc9999','#999999','#cc3366','#ff66ff',
                        '#996699','#ccccff','#3366cc','#66ccff','#66cccc','#66cc66','#99ff66','#00ff00',
                        '#666600','#cc9900','#993333','#666666','#ffccff','#ff33ff','#663366','#9999ff',
                        '#000066','#3399ff','#339999','#99ff99','#99ff33','#33cc00','#999933','#ff9900',
                        '#cc6666','#333333','#cc6699','#ff00ff','#660099','#9999cc','#000033','#003399',
                        '#336666','#66ff66','#00ff33','#339900','#cccc66','#cc6600','#ffcccc','#993366',
                        '#cc0099','#9933cc','#6666cc','#0000ff','#0099ff','#006666','#339933','#33ff33',
                        '#99cc66','#666633','#993300','#ff3333','#660033','#990066','#660066','#6666ff',
                        '#000099','#33ccff','#003333','#99cc99','#00cc33','#669933','#999966','#cc6633',
                        '#cc3333','#cc3399','#cc66cc','#9900ff','#666699','#0033cc','#00ccff','#00ffcc',
                        '#66ff99','#33cc33','#99cc33','#cccc99','#663300','#ff6666','#ff99cc','#cc33cc',
                        '#9933ff','#333366','#0000cc','#99ffff','#33ffcc','#33ff99','#66ff33','#336600',
                        '#ffffcc','#ff9966','#660000','#ff66cc','#cc99ff','#9966cc','#333399','#336699',
                        '#66ffff','#33cc99','#33cc66','#009900','#669900','#ffff99','#ff6633','#990000',
                        '#ff99ff','#cc66ff','#330033','#330099','#0066cc','#33ffff','#00cc99','#00cc66',
                        '#66cc33','#99cc00','#ffff66','#ff9933','#cc0000','#ff6699','#cc33ff','#663399',
                        '#3300cc','#99ccff','#00ffff','#66ffcc','#66cc99','#006600','#ccff66','#ffff33',
                        '#ff6600','#ff0000','#cc0066','#993399','#6633cc','#3300ff','#6699ff','#00cccc',
                        '#99ffcc','#009966','#003300','#ccff33','#ffff00','#cc3300','#ff3300','#000000'];

    var _chartGetColor = function(pos) {
        var idx = pos % _chartColors.length;
        return _chartColors[idx];
    }

    var date = new Date( 1369720268 * 1000 );

    var methods = {
        init: function(options) {
            return this.each(function(){
                var $this = $(this);
                var data = $this.data('chart');
                if (!data) {
                    options = $.extend(true, {
                        label: "",  
                        xLegend: "",
                        yLegend: "",
                        width: 600,
                        values: []
                    }, options);   
                    $(this).data('chart', options);
                }
                _chartDraw(this, data)
            });
        },
        update: function(values) {
            return this.each(function(){
                var $this = $(this);
                var data = $this.data('chart');
                data.chart.remove();
        
        }
    }

    $.fn.chart = function(method) {
        if (methods[method]) {
            return methods[method].apply(this, Array.prototype.slice.call(arguments, 1));
        } else if (typeof method === 'object' || !method) {
            return methods.init.apply(this, arguments);
        } else {
            $.error('Method ' +  method + ' does not exist on jQuery.chart');
        }  
        return this;
    }

})(jQuery);
