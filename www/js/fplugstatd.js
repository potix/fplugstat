var fplugstatd = {
    select_view: function(body_id, menu_id) {
        $("#body-fplug-realtime").hide();
        $("#sidebar-fplug-realtime").css("color","black");
        $("#body-fplug-hourly").hide();
        $("#sidebar-fplug-hourly").css("color","black");
        $("#body-fplug-control").hide();
        $("#sidebar-fplug-control").css("color","black");
        $("#" + body_id).show();
        $("#" + menu_id).css("color","red");
    },
    current_control_device: null,
    realtime_interval: null,
    hourly_interval: null,
    realtime_temperature_values: {},
    realtime_humidity_values: {},
    realtime_illuminance_values: {},
    realtime_watt_values: {},
    hourly_temperature_values: {},
    hourly_humidity_values: {},
    hourly_illuminance_values: {},
    hourly_watt_values: {},
    date_format: function(date) {
       var year = date.getFullYear();  
       var month = ("00" + (date.getMonth() + 1)).slice(-2);  
       var day = ("00" + date.getDate()).slice(-2);  
       var str = "" + year +  month +  day + "00" + "00" + "00";  
       return str
    },
    get_realtime_values: function() {
        for (var j = 0; j < fplugstatd.devicies.length; j++) {
            var data = { address : fplugstatd.devicies[j].address };
            if (fplugstatd.select_start_end_realtime) {
                 data.start = fplugstatd.date_format(fplugstatd.select_start_realtime_date);
                 data.end = fplugstatd.date_format(fplugstatd.select_end_realtime_date);
            }
            $.ajax({
                method: "POST",
                url: "/api/device/realtime",
                data: data,
                context: fplugstatd.devicies[j],
                cache: false
            }).done(function(msg){
                fplugstatd.realtime_temperature_values[this.name] = [];
                fplugstatd.realtime_humidity_values[this.name] = [];
                fplugstatd.realtime_illuminance_values[this.name] = [];
                fplugstatd.realtime_watt_values[this.name] = [];
                for (var i = 0; i < msg.length; i++ ) {
                    elm = msg[i];
                    fplugstatd.realtime_temperature_values[this.name].push([elm.time * 1000, elm.temperature]);
                    fplugstatd.realtime_humidity_values[this.name].push([elm.time * 1000, elm.humidity]);
                    fplugstatd.realtime_illuminance_values[this.name].push([elm.time * 1000, elm.illuminance]);
                    fplugstatd.realtime_watt_values[this.name].push([elm.time * 1000, elm.rwatt]);
                }
                fplugstatd.draw_realtime_temperature_chart();
                fplugstatd.draw_realtime_humidity_chart();
                fplugstatd.draw_realtime_illuminance_chart();
                fplugstatd.draw_realtime_watt_chart();
            });
        }
    },
    get_hourly_power_values: function() {
        for (var j = 0; j < fplugstatd.devicies.length; j++) {
            var data = { address : fplugstatd.devicies[j].address };
            if (fplugstatd.select_end_hourly) {
                 data.end = fplugstatd.date_format(fplugstatd.select_end_hourly_date);
            }
            $.ajax({
                method: "POST",
                url: "/api/device/hourly/power/total",
                data: data,
                context: fplugstatd.devicies[j],
                cache: false
            }).done(function(msg){
                fplugstatd.hourly_watt_values[this.name] = [];
                for (var i = 0; i < msg.length; i++ ) {
                    elm = msg[i];
                    fplugstatd.hourly_watt_values[this.name].push([23 - elm.index, elm.watt]);
                }
                fplugstatd.draw_hourly_watt_chart();
            });
        }
    },
    get_hourly_other_values: function() {
        for (var j = 0; j < fplugstatd.devicies.length; j++) {
            var data = { address : fplugstatd.devicies[j].address };
            if (fplugstatd.select_end_hourly) {
                 data.end = fplugstatd.date_format(fplugstatd.select_end_hourly_date);
            }
            $.ajax({
                method: "POST",
                url: "/api/device/hourly/other",
                data: data,
                context: fplugstatd.devicies[j],
                cache: false
            }).done(function(msg){
                fplugstatd.hourly_temperature_values[this.name] = [];
                fplugstatd.hourly_humidity_values[this.name] = [];
                fplugstatd.hourly_illuminance_values[this.name] = [];
                for (var i = 0; i < msg.length; i++ ) {
                    elm = msg[i];
                    fplugstatd.hourly_temperature_values[this.name].push([23 - elm.index, elm.temperature]);
                    fplugstatd.hourly_humidity_values[this.name].push([23 - elm.index, elm.humidity]);
                    fplugstatd.hourly_illuminance_values[this.name].push([23 - elm.index, elm.illuminance]);
                }
                fplugstatd.draw_hourly_temperature_chart();
                fplugstatd.draw_hourly_humidity_chart();
                fplugstatd.draw_hourly_illuminance_chart();
            });
        }
    },
    device_reset: function() {
        $.ajax({
            method: "POST",
            url: "/api/device/reset",
            data: { address : fplugstatd.current_control_device.address },
            cache: false
        }).done(function(msg){
            alert(fplugstatd.current_control_device.name + "をリセットしました。");
        }).fail(function(e){
            alert(fplugstatd.current_control_device.name + "のリセット中にエラーが発生しました。");
        });
    },
    device_set_datetime: function() {
        $.ajax({
            method: "POST",
            url: "/api/device/datetime",
            data: { address : fplugstatd.current_control_device.address },
            cache: false
        }).done(function(msg){
            alert(fplugstatd.current_control_device.name + "の時刻を設定しました。");
        }).fail(function(e){
            alert(fplugstatd.current_control_device.name + "の時刻設定中にエラーが発生しました。");
        });
    },
    start_hourly_power_store: function() {
        $.ajax({
            method: "POST",
            url: "/api/device/hourly/power/total",
            data: { address : fplugstatd.current_control_device.address, init : 1 },
            cache: false
        }).done(function(msg){
            fplugstatd.hourly_watt_values = [];
            for (var i = 0; i < msg.length; i++ ) {
                elm = msg[i];
                fplugstatd.hourly_watt_values.push([elm.index, elm.watt]);
            }
            fplugstatd.draw_hourly_watt_chart();
            alert(fplugstatd.current_control_device.name + "の測定値の蓄積を開始しました。");
        }).fail(function(e) {
            alert(fplugstatd.current_control_device.name + "の測定値の蓄積を開始中にエラーが発生しました。");
        });
    },
    realtime_temperature_chart: null,
    draw_realtime_temperature_chart: function() {
        var series = [];
        for (name in fplugstatd.realtime_temperature_values) {
           series.push({ name: name, data : fplugstatd.realtime_temperature_values[name] });
        }
        var options = {
            chart : {
                renderTo : "body-fplug-realtime-temperature-chart",
                width: 800,
                type: 'line',
                zoomType: "x",
                panning: true,
                panKey: 'shift'
            },
            rangeSelector : { selected : 1 },
            title : { text: "温度 (リアルタイム)" },
            xAxis : {
                type: "datetime",
                title: {text : "時間"}},
            yAxis : { title: {text : "温度(℃)"}, },
            series: series
        }
        fplugstatd.realtime_temperature_chart = new Highcharts.Chart(options);
    },
    realtime_humidity_chart: null,
    draw_realtime_humidity_chart: function() {
        var series = [];
        for (name in fplugstatd.realtime_humidity_values) {
           series.push({ name: name, data : fplugstatd.realtime_humidity_values[name] });
        }
        var options = {
            chart : { 
                renderTo : "body-fplug-realtime-humidity-chart",
                width: 800,
                type: 'line',
                zoomType: "x",
                panning: true,
                panKey: 'shift'
            },
            rangeSelector : { selected : 1 },
            title : { text: "湿度 (リアルタイム)" },
            xAxis : {
                type: "datetime",
                title: {text : "時間"}
            },
            yAxis : {
                title: {text : "湿度(％)"},
                allowDecimals: false
            },
            series: series
        }
        fplugstatd.realtime_humidity_chart = new Highcharts.Chart(options);
    },
    realtime_illuminance_chart: null,
    draw_realtime_illuminance_chart: function() {
        var series = [];
        for (name in fplugstatd.realtime_illuminance_values) {
           series.push({ name: name, data : fplugstatd.realtime_illuminance_values[name] });
        }
        var options = {
            chart : {
                renderTo : "body-fplug-realtime-illuminance-chart",
                width: 800,
                type: 'line',
                zoomType: "x",
                panning: true,
                panKey: 'shift'
            },
            rangeSelector : { selected : 1 },
            title : { text: "照度 (リアルタイム)" },
            xAxis : {
                type: "datetime",
                title: {text : "時間"}
            },
            yAxis : {
                title: {text : "照度(ルクス)"},
                allowDecimals: false
            },
            series: series
        }
        fplugstatd.realtime_illuminance_chart = new Highcharts.Chart(options);
    },
    realtime_watt_chart: null,
    draw_realtime_watt_chart: function() {
        var series = [];
        for (name in fplugstatd.realtime_watt_values) {
           series.push({ name: name, data: fplugstatd.realtime_watt_values[name] });
        }
        var options = {
            chart : {
                renderTo : "body-fplug-realtime-watt-chart",
                width: 800,
                type: 'line',
                zoomType: "x",
                panning: true,
                panKey: 'shift'
            },
            rangeSelector : { selected : 1 },
            title : { text: "電力 (リアルタイム)" },
            xAxis : {
                type: "datetime",
                title: {text : "時間"} 
            },
            yAxis : {title: {text : "電力(W)"}},
            series: series
        }
        fplugstatd.realtime_watt_chart = new Highcharts.Chart(options);
    },
    hourly_temperature_chart: null,
    draw_hourly_temperature_chart: function() {
        var series = [];
        for (name in fplugstatd.hourly_temperature_values) {
           series.push({ name: name, data: fplugstatd.hourly_temperature_values[name] });
        }
        var options = {
            chart : {
                renderTo : "body-fplug-hourly-temperature-chart",
                width: 800,
                type: 'line',
                zoomType: "x",
                panning: true,
                panKey: 'shift'
            },
            rangeSelector : { selected : 1 },
            title : { text: "温度 (時間毎)" },
            xAxis : {
                title: {text : "24時間分のデータ(左が古い)"},
                allowDecimals: false,
                reversed: true
            },
            yAxis : { title: {text : "温度(℃)"} },
            series: series
        }
        fplugstatd.hourly_temperature_chart = new Highcharts.Chart(options);
    },
    hourly_humidity_chart: null,
    draw_hourly_humidity_chart: function() {
        var series = [];
        for (name in fplugstatd.hourly_humidity_values) {
           series.push({ name: name, data: fplugstatd.hourly_humidity_values[name] });
        }
        var options = {
            chart : {
                renderTo : "body-fplug-hourly-humidity-chart",
                width: 800,
                type: 'line',
                zoomType: "x",
                panning: true,
                panKey: 'shift'
            },
            rangeSelector : { selected : 1 },
            title : { text: "湿度 (時間毎)" },
            xAxis : {
                title: {text : "24時間分のデータ(左が古い)"},
                allowDecimals: false,
                reversed: true
            },
            yAxis : {
                title: {text : "湿度(%)"},
                allowDecimals: false
            },
            series: series
        }
        fplugstatd.hourly_humidity_chart = new Highcharts.Chart(options);
    },
    hourly_illuminance_chart: null,
    draw_hourly_illuminance_chart: function() {
        var series = [];
        for (name in fplugstatd.hourly_illuminance_values) {
           series.push({ name: name, data: fplugstatd.hourly_illuminance_values[name] });
        }
        var options = {
            chart : {
                renderTo : "body-fplug-hourly-illuminance-chart",
                width: 800,
                type: 'line',
                zoomType: "x",
                panning: true,
                panKey: 'shift'
            },
            rangeSelector : { selected : 1 },
            title : { text: "照度 (時間毎)" },
            xAxis : {
                title: {text : "24時間分のデータ(左が古い)"},
                allowDecimals: false,
                reversed: true
            },
            yAxis : {
                title: {text : "照度(ルクス)"},
                allowDecimals: false
            },
            series: series
        }
        fplugstatd.hourly_illuminance_chart = new Highcharts.Chart(options);
    },
    hourly_watt_chart: null,
    draw_hourly_watt_chart: function() {
        var series = [];
        for (name in fplugstatd.hourly_watt_values) {
           series.push({ name: name, data: fplugstatd.hourly_watt_values[name] });
        }
        var options = {
            chart : {
                renderTo : "body-fplug-hourly-watt-chart",
                width: 800,
                type: 'line',
                zoomType: "x",
                panning: true,
                panKey: 'shift'
            },
            rangeSelector : { selected : 1 },
            title : { text: "電力量 (時間毎)" },
            xAxis : {
                title: {text : "24時間分のデータ(左が古い)"},
                allowDecimals: false,
                reversed: true
            },
            yAxis : {title: {text : "電力量(Wh)"}},
            series: series
        }
        fplugstatd.hourly_watt_chart = new Highcharts.Chart(options);
    },
    start_polling: function(type) {
        if (fplugstatd.realtime_interval != null) {
            clearInterval(fplugstatd.realtime_interval);
            fplugstatd.realtime_interval = null;
        }
        if (fplugstatd.hourly_interval != null) {
            clearInterval(fplugstatd.hourly_interval);
            fplugstatd.hourly_interval = null;
        }
        if (type == "realtime") {
            fplugstatd.realtime_interval = setInterval(function(){
                fplugstatd.get_realtime_values();
            }, 1000 * 60);
            fplugstatd.get_realtime_values();
        } else if (type == "hourly") {
            fplugstatd.hourly_interval = setInterval(function(){
                fplugstatd.get_hourly_power_values();
                fplugstatd.get_hourly_other_values();
            }, 1000 * 600);
            fplugstatd.get_hourly_power_values();
            fplugstatd.get_hourly_other_values();
        }
    },
    select_start_end_realtime: false,
    select_start_realtime_date: null,
    select_end_realtime_date: null,
    select_end_hourly: false,
    select_end_hourly_date: null
}

$(document).ready(function(){
    Highcharts.setOptions({
      global: {
        useUTC: false   // GMTではなくJSTを使う
      },
      lang: { 
        rangeSelectorZoom: '表示範囲',
        resetZoom: '表示期間をリセット',
        resetZoomTitle: '表示期間をリセット',
        rangeSelectorFrom: '表示期間',
        rangeSelectorTo: '〜',
        printButtonTitle: 'チャートを印刷',
        exportButtonTitle: '画像としてダウンロード',
        downloadJPEG: 'JPEG画像でダウンロード',
        downloadPDF: 'PDF文書でダウンロード',
        downloadPNG: 'PNG画像でダウンロード',
        downloadSVG: 'SVG形式でダウンロード',
        months: ['1月', '2月', '3月', '4月', '5月', '6月', '7月', '8月', '9月', '10月', '11月', '12月'],
        weekdays: ['日', '月', '火', '水', '木', '金', '土'],
        numericSymbols: null   // 1000を1kと表示しない
      }
    });
    $("#body-fplug-realtime-tabs").tabs();
    $("#body-fplug-hourly-tabs").tabs();
    $("#select-start-realtime-datepicker").datepicker({
        showAnim: "slideDown",
        showOn: "both",
        dateFormat: "yy/mm/dd"
    });
    $("#select-end-realtime-datepicker").datepicker({
        showAnim: "slideDown",
        showOn: "both",
        dateFormat: "yy/mm/dd"
    });
    $("#select-end-hourly-datepicker").datepicker({
        showAnim: "slideDown",
        showOn: "both",
        dateFormat: "yy/mm/dd"
    });
    $("#body-fplug-control-device").change(function() {
        fplugstatd.current_control_device = {
            address : $("#body-fplug-control-device").val(),
            name:  $("#body-fplug-control-device option:selected").text()
        };
	return false;
    });
    $("#sidebar-fplug-realtime").click(function(event) {
        fplugstatd.select_view("body-fplug-realtime", $(event.target).attr("id"));
        fplugstatd.start_polling("realtime");
	return false;
    });
    $("#sidebar-fplug-hourly").click(function(event) {
        fplugstatd.select_view("body-fplug-hourly", $(event.target).attr("id"));
        fplugstatd.start_polling("hourly");
	return false;
    });
    $("#sidebar-fplug-control").click(function(event) {
        fplugstatd.select_view("body-fplug-control", $(event.target).attr("id"));
        if (fplugstatd.hourly_interval != null) {
            clearInterval(fplugstatd.hourly_interval);
            fplugstatd.hourly_interval = null;
        }
        if (fplugstatd.realtime_interval != null) {
            clearInterval(fplugstatd.realtime_interval);
            fplugstatd.realtime_interval = null;
        }
	return false;
    });
    $("#body-fplug-control-reset").click(function(event) {
        fplugstatd.device_reset();
        return false;
    });
    $("#body-fplug-control-powerstart").click(function(event) {
        fplugstatd.start_hourly_power_store();
        return false;
    });
    $("#body-fplug-control-datetimesetting").click(function(event) {
        fplugstatd.device_set_datetime();
        return false;
    });
    $("#refresh-realtime-button").click(function(event) {
	if ($("#select-start-end-realtime").prop('checked')) {
            var start =  $("#select-start-realtime-datepicker").datepicker('getDate');
            var end =  $("#select-end-realtime-datepicker").datepicker('getDate');
            if (start == null || end == null) {
                alert("開始日と終了日を入れてください");
                return false;
            }
            fplugstatd.select_start_end_realtime = true;
            fplugstatd.select_start_realtime_date = new Date(start);
            fplugstatd.select_end_realtime_date = new Date(end);
        } else {
            fplugstatd.select_start_end_realtime = false;
            fplugstatd.select_start_realtime_date = null;
            fplugstatd.select_end_realtime_date = null;
        }
        fplugstatd.start_polling("realtime");
	return false;
    });
    $("#refresh-hourly-button").click(function(event) {
	if ($("#select-end-hourly").prop('checked')) {
            var end =  $("#select-end-hourly-datepicker").datepicker('getDate');
            if (end == null) {
                alert("終了日を入れてください");
                return false;
            }
            fplugstatd.select_end_hourly = true;
            fplugstatd.select_end_hourly_date = new Date(end);
        } else {
            fplugstatd.select_end_hourly = false;
            fplugstatd.select_end_hourly_date = null;
        }
        fplugstatd.start_polling("hourly");
        return false;
    });
    fplugstatd.select_view("body-fplug-realtime", "sidebar-fplug-realtime");
    $.ajax({
        method: "GET",
        url: "/api/devicies",
        cache: false
    }).done(function(msg) {
        fplugstatd.devicies = msg;
        for (var i = 0; i < msg.length; i++) {
            $("#body-fplug-control-device").append($("<option>").val(msg[i].address).text(msg[i].name));
        }
        fplugstatd.current_control_device = {
            address : $("#body-fplug-control-device").val(),
            name:  $("#body-fplug-control-device option:selected").text()
        };
        fplugstatd.start_polling("realtime");
    }); 
});

