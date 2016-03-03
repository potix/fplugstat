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

    var _cGetColor = function(pos) {
        var idx = pos % _chartColors.length;
        return _chartColors[idx];
    }

    var _cGetStringLength = function(str) {
        len = 0;
        for (var i = 0; i < str.length; i++) {
            if (str.charCodeAt(i) < 256) {
                len += 1;
            } else {
                len += 2;
            }
        }
        return len;
    }

    //var date = new Date( 1369720268 * 1000 );

    var _cDrawTopContainer = function(target, selector, info) {
        var obj = d3.selectAll(target)
                    .selectAll(selector)
                    .data(info);
        obj.attr("class", function(d) { return d.class; });
        obj.enter()
           .append("div")
           .attr("class", function(d) { return d.class; });
        obj.exit().remove();
        return obj;
    }

    var _cDrawChartTitle = function(target, selector, info) {
        var obj = d3.selectAll(target)
                    .selectAll(selector)
                    .data(info);
        obj.attr("class", function(d) { return d.class; })
           .text(function(d) { return d.title; });
        obj.enter()
           .append("div")
           .attr("class", function(d) { return d.class; })
           .text(function(d) { return d.title; });
        obj.exit().remove();
        return obj;
    }

    var _cDrawChartTop = function(target, selector, info) {
        var obj = d3.selectAll(target)
                    .selectAll(selector)
                    .data(info);
        obj.attr("class", function(d) { return d.class })
           .attr("width", function(d) { return d.width })
           .attr("height", function(d) { return d.height })
           .attr("viewBox", function(d) { return d.viewBox });
        obj.enter()
           .append("svg")
           .attr("class", function(d) { return d.cls })
           .attr("width", function(d) { return d.width })
           .attr("height", function(d) { return d.height })
           .attr("viewBox", function(d) { return d.viewBox });
        obj.exit().remove();
        return obj;
    }

    var _cDrawChartChild = function(target, selector, info) {
        var obj = d3.selectAll(target)
                    .selectAll(selector)
                    .data(info);
        obj.attr("class", function(d) { return d.class })
           .attr("preserveAspectRatio", function(d) { return d.preserveAspectRatio})
           .attr("x", function(d) { return d.x })
           .attr("y", function(d) { return d.y })
           .attr("width", function(d) { return d.width })
           .attr("height", function(d) { return d.height })
           .attr("viewBox", function(d) { return d.viewBox });
        obj.enter()
           .append("svg")
           .attr("class", function(d) { return d.cls })
           .attr("preserveAspectRatio", function(d) { return d.preserveAspectRatio})
           .attr("x", function(d) { return d.x })
           .attr("y", function(d) { return d.y })
           .attr("width", function(d) { return d.width })
           .attr("height", function(d) { return d.height })
           .attr("viewBox", function(d) { return d.viewBox});
        obj.exit().remove();
        return obj;
    }

    var _tcDrawChartMainAxisX = function(target, selectorMain, selectorAxis, info) {
        var obj = d3.selectAll(target)
                    .selectAll(selectorMain)
                    .selectAll(selectorAxis)
                    .data(info);
        obj.attr("class", function(d){ return d.class; })
           .attr("transform", function(d){ return "translate(0," + d.height + ")"; });
           .call(function(d){ return d.axisX; })
        obj.enter()
           .append("g")
           .attr("class", function(d){ return d.class; });
           .attr("transform", function(d) { return "translate(0," + d.height + ")"; })
           .call(function(d){ return d.axisX; })
        obj.exit().remove();
        return obj;
    }

    var _tcDrawChartMainUnitX = function(target, selector, info) {
        var obj = d3.selectAll(target)
                    .selectAll(selector)
                    .data(info);
        obj.attr("class", function(d){ return d.class; })
           .attr("transform", rotate(-90))
           .attr("x", function(d){ return d.x; })
           .attr("y", function(d){ return d.y; })
           .style("text-anchor", "end")
           .style("font-size", function(d){ return d.fontSize; })
           .text(function(d){ return d.unitX });
        obj.enter()
           .append("text")
           .attr("class", function(d){ return d.class; })
           .attr("transform", rotate(-90))
           .attr("s", function(d){ return d.x; })
           .attr("y", function(d){ return d.y; })
           .style("text-anchor", "end")
           .text(function(d){ return d.unitX });
           .style("font-size", function(d){ return d.fontSize; })
        obj.exit().remove();
        return obj;
    }

    var _tcDrawChartMainAxisY = function(target, selectorMain, selectorAxis, info) {
        var obj = d3.selectAll(target)
                    .selectAll(selectorMain)
                    .selectAll(selectorAxis)
                    .data(info);
        obj.attr("class", function(d){ return d.class; })
           .call(function(d){ return d.axisY; })
        obj.enter()
           .append("g")
           .attr("class", function(d){ return d.class; });
           .call(function(d){ return d.axisY; })
        obj.exit().remove();
        return obj;
    }

    var _tcDrawChartMainUnitY = function(target, selector, info) {
        var obj = d3.selectAll(target)
                    .selectAll(selector)
                    .data(info);
        obj.attr("class", function(d){ return d.class; })
           .attr("transform", rotate(-90));
           .attr("x", function(d){ return d.x; } );
           .attr("y", function(d){ return d.y; } );
           .style("text-anchor", "end");
           .style("font-size", function(d){ return d.fontSize; })
           .text(function(d){ return d.unitX });
        obj.enter()
           .append("text")
           .attr("class", function(d){ return d.class; })
           .attr("transform", rotate(-90));
           .attr("x", function(d){ return d.x; });
           .attr("y", function(d){ return d.y; });
           .style("text-anchor", "end");
           .style("font-size", function(d){ return d.fontSize; })
           .text(function(d){ return d.unitX });
        obj.exit().remove();
        return obj;
    }

    var _tcDrawChartMainLineContainer = function(target, selectorMain, selectorLineContainer, info) {
        var obj = d3.selectAll(target)
                    .selectAll(selectorMain)
                    .selectAll(selectorLine)
                    .data(info);
        obj.attr("class", function(d){ return d.class; })
        obj.enter()
           .append("g")
           .attr("class", function(d){ return d.class; });
        obj.exit().remove();
        return obj;
    }

    var _tcDrawChartMainLine = function(target, selectorMain, selectorLineContainer, selectorLine, info) {
        var obj = d3.selectAll(target)
                    .selectAll(selectorMain)
                    .selectAll(selectorLineContainer)
                    .selectAll(selectorLine)
                    .data(info);
        obj.attr("class", function(d){ return d.class; })
           .attr("d", afunction(d) { return line(d.values); })
           .attr("data-legend",function(d) { return d.legend})
           .style("stroke",function(d) { return d.color });
        obj.enter()
           .append("path")
           .attr("class", function(d){ return d.class; })
           .attr("d", afunction(d) { return line(d.values); })
           .attr("data-legend",function(d) { return d.legend})
           .style("stroke",function(d) { return d.color });
        obj.exit().remove();
        return obj;
    }

    var _chartDraw = function(self, data)  {
        // 判例の最大文字列とっとく
        var maxLegendLen = 0;
        for (var i = 0;  i < data.lines.length; i++ ) {
            var len = _tcGetStringLength(data.lines[i].legend);
            if (maxLegendLen < len) {
                maxLegendLen = len;
            }
        }
        // 最大最小値調べとく
        var xMax = null;
        var yMax = null;
        var xMin = null;
        var yMin = null;
        for (var i = 0;  i < data.lines.length; i++ ) {
            var max = d3.max(data.lines[i].values, function(d){ return d.x });
            if (xMax == null || xMax < max) {
                xMax = max;
            }
            max = d3.max(data.lines[i].values, function(d){ return d.y });
            if (yMax == null || yMax < max) {
                yMax = max;
            }
            var min = d3.min(data.lines[i].values, function(d){ return d.x });
            if (xMin == null || xMin > min) {
                xMin = min;
            }
            min = d3.min(data.lines[i].values, function(d){ return d.y });
            if (yMin == null || yMin > min) {
                yMin = min;
            }
        }
        // define
        var unitFontSize = 16;
        var yPadding = 8;
        var xPadding = 8;
        var unitXSizeX = _tcGetStringLength(data.unitX) * unitFontSize;

        // mainのcanvas
        var mainWidth = data.width;
        var mainHeight = (data.Height/5) * 4;
        var mainOffsetX = 0
        var mainOffsetY = 0;

        // naviのcanvas
        var naviWidth =  data.width;
        var naviHeight = data.Height/5
        var naviOffsetX = 0;
        var naviOffsetY = mainHeight

	// x Scale
        var scaleX = d3.scale.linear()
                       .domain([xMin, xMax)])
                       .range([0, mainWidth]);
	// x Scale
        var scaleY = d3.scale.linear()
                       .domain([yMin, yMax])
                       .range([mainHeight, 0]);

        // x axis
        var axisX = d3.svg.axis()
                          .scale(xScale)
                          .orient("bottom")
                          .innerTickSize(-mainHeight)
                          .outerTickSize(0)
                          .tickPadding(xPadding);
        // y axis
        var axisY = d3.svg.axis()
                          .scale(yScale)
                          .orient("left")
                          .innerTickSize(-mainCanvasWidth)
                          .outerTickSize(0)
                          .tickPadding(yPadding);
        // line
        var line = d3.svg.line()
                     .interpolate("basis")
                     .x(function(d) { return xScale(d.x); })
                     .y(function(d) { return yScale(d.y); });

        // top container
        var containerInfo = [ { class : "chartContainer" } ]
        var topContainerDiv = _cDrawTopContainer(self, "div.chartContainer", containerInfo);
        // title
        var titleInfo = [ { class: "chartTitle", title:data.title } ];
        var titleDiv = _cDrawChartTitle(topContainerDiv, "div.chartTitle", titleInfo);
        // chart top 
        var chartTopInfo = [
            {
                class: "chartTop",
                width:data.width,
                height:data.height,
                viewBox: viewBox: "" + 0 + " " + 0 + " " + data.width + " " + data.height
            }
        ];
        var chartTopSvg = _cDrawChartTop(topContainerDiv, "svg.chartTop", chartTopInfo);
        // chart child
        var chartChildInfo = [
            {
                class: "chartChildMain chartChild",
                x: mainOffsetX,
                y: mainOffsetY,
                width: mainWidth,
                height: mainHeight,
                preserveAspectRatio: "xMidYMid",
                viewBox: viewBox: "" + 0 + " " + 0 + " " + mainWidth + " " + mainHeight
            },
            {
                class: "chartChildNavi chartChild",
                x: naviOffsetX,
                y: naviOffsetY,
                width: naviWidth,
                height: naviHeight,
                preserveAspectRatio: "xMidYMid",
                viewBox: viewBox: "" + 0 + " " + 0 + " " + naviWidth + " " + naviHeight
            }
        ];
        var chartChildSvg = _cDrawChartChild(chartTopSvg, "svg.chartChild", chartChildInfo);
        // draw axis x
        var chartAxisXInfo = [
            {
                class: "chartAxisX chartAxis",
                height: mainHeight,
                axisX: axisX
            }
        ]
        var chartAxisXG = _tcDrawChartMainAxisX(chartTopSvg, "svg.chartChildMain", "g.chartAxisX", info);
        // draw unit x
        var chartUnitXInfo = [
            {
                class: "chartUnitX chartUnit",
                x: mainHeight - unitXSizeX - xPadding,
                y: yPadding ,
                fontSize: unitFontSize + "px",
                unitX: data.unitX
            }
        ]
        var chartUnitXText = _tcDrawChartMainUnitX(chartAxisXG, "text.chartUnitX", info);
        // draw axis y
        var chartAxisYInfo = [
            {
                class: "chartAxisY chartAxis",
                axisX: axisY
            }
        ]
        var chartAxisYG = _tcDrawChartMainAxisY(chartTopSvg, "svg.chartChildMain", "g.chartAxisY", info);
        // draw unit x
        var chartUnitYInfo = [
            {
                class: "chartUnitY chartUnit",
                x: xPadding,
                y: yPadding ,
                fontSize: unitFontSize + "px",
                unitX: data.unitY
            }
        ]
        var chartUnitYText = _tcDrawChartMainUnitY(chartAxisYG, "text.chartUnitY", info);
        // draw line container
        
 
        






         
    }
    
    var methods = {
        init: function(options) {
            var $this = $(this);
            var data = $this.data('chart');
            if (!data) {
                options = $.extend(true, {
                    title: "title",
                    width: 800,
                    height: 500,
                    labelX: "time",
                    labelY: "point",
                    lines: [
                        {
                            legend: "legend",
                            values: [
                                {
                                    x:1,
                                    y:2,
                                },
                                {
                                    x:2,
                                    y:3,
                                }
                            ]
                        }
                    ];
                }, options);   
                $(this).data('chart', options);
            }
            _chartDraw(this, data);
        },
        update: function(multiValues) {
            var $this = $(this);
            var data = $this.data('chart');
            if (!data) {
                return this;
            }
            _chartRemove(this, data);
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
