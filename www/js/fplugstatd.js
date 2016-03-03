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
    current_device: null,
    realtime_interval: null,
    hourly_interval: null,
    realtime_temperature_values: [],
    realtime_humidity_values: [],
    realtime_illuminance_values: [],
    realtime_watt_values: [],
    hourly_temperature_values: [],
    hourly_humidity_values: [],
    hourly_illuminance_values: [],
    hourly_watt_values: [],
    get_realtime_values: function() {
        $.ajax({
            method: "POST",
            url: "/api/device/realtime",
            data: { address : fplugstatd.current_device },
            cache: false
        }).done(function(msg){
            fplugstatd.realtime_temperature_values = [];
            fplugstatd.realtime_humidity_values = [];
            fplugstatd.realtime_illuminance_values = [];
            fplugstatd.realtime_watt_values = [];
            for (var i = 0; i < msg.length; i++ ) {
                elm = msg[i];
                fplugstatd.realtime_temperature_values.push([elm.time * 1000, elm.temperature]);
                fplugstatd.realtime_humidity_values.push([elm.time * 1000, elm.humidity]);
                fplugstatd.realtime_illuminance_values.push([elm.time * 1000, elm.illuminance]);
                fplugstatd.realtime_watt_values.push([elm.time * 1000, elm.rwatt]);
            }
            fplugstatd.draw_realtime_temperature_chart();
            fplugstatd.draw_realtime_humidity_chart();
            fplugstatd.draw_realtime_illuminance_chart();
            fplugstatd.draw_realtime_watt_chart();
        });
    },
    get_hourly_power_values: function() {
        $.ajax({
            method: "POST",
            url: "/api/device/hourly/power/total",
            data: { address : fplugstatd.current_device },
            cache: false
        }).done(function(msg){
            fplugstatd.hourly_watt_values = [];
            for (var i = 0; i < msg.length; i++ ) {
                elm = msg[i];
                fplugstatd.hourly_watt_values.push([23 - elm.index, elm.watt]);
            }
            fplugstatd.draw_hourly_watt_chart();
        });
    },
    get_hourly_other_values: function() {
        $.ajax({
            method: "POST",
            url: "/api/device/hourly/other",
            data: { address : fplugstatd.current_device },
            cache: false
        }).done(function(msg){
            fplugstatd.hourly_temperature_values = [];
            fplugstatd.hourly_humidity_values = [];
            fplugstatd.hourly_illuminance_values = [];
            for (var i = 0; i < msg.length; i++ ) {
                elm = msg[i];
                fplugstatd.hourly_temperature_values.push([23 - elm.index, elm.temperature]);
                fplugstatd.hourly_humidity_values.push([23 - elm.index, elm.humidity]);
                fplugstatd.hourly_illuminance_values.push([23 - elm.index, elm.illuminance]);
            }
            fplugstatd.draw_hourly_temperature_chart();
            fplugstatd.draw_hourly_humidity_chart();
            fplugstatd.draw_hourly_illuminance_chart();
        });
    },
    device_reset: function() {
        $.ajax({
            method: "POST",
            url: "/api/device/reset",
            data: { address : fplugstatd.current_device },
            cache: false
        }).done(function(msg){
            alert("リセットしました。");
        }).fail(function(e){
            alert("リセット中にエラーが発生しました。");
        });
    },
    device_set_datetime: function() {
        $.ajax({
            method: "POST",
            url: "/api/device/datetime",
            data: { address : fplugstatd.current_device },
            cache: false
        }).done(function(msg){
            alert("時刻を設定しました。");
        }).fail(function(e){
            alert("時刻設定中にエラーが発生しました。");
        });
    },
    start_hourly_power_store: function() {
        $.ajax({
            method: "POST",
            url: "/api/device/hourly/power/total",
            data: { address : fplugstatd.current_device, init : 1 },
            cache: false
        }).done(function(msg){
            fplugstatd.hourly_watt_values = [];
            for (var i = 0; i < msg.length; i++ ) {
                elm = msg[i];
                fplugstatd.hourly_watt_values.push([23 - elm.index, elm.watt]);
            }
            fplugstatd.draw_hourly_watt_chart();
            alert("積算電力の蓄積を開始しました。");
        }).fail(function(e)) {
            alert("積算電力蓄積の開始中にエラーが発生しました。")
        });
    },
    realtime_temperature_chart: null,
    draw_realtime_temperature_chart: function() {
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
            yAxis : { title: {text : "温度(℃)"} },
            series: [
               {
                  name: fplugstatd.current_device,
                  data: fplugstatd.realtime_temperature_values,
               }
            ]
        }
        fplugstatd.realtime_temperature_chart = new Highcharts.Chart(options);
    },
    realtime_humidity_chart: null,
    draw_realtime_humidity_chart: function() {
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
            yAxis : {title: {text : "湿度(％)"}},
            series: [
               {
                  name: fplugstatd.current_device,
                  data: fplugstatd.realtime_humidity_values,
               }
            ]
        }
        fplugstatd.realtime_humidity_chart = new Highcharts.Chart(options);
    },
    realtime_illuminance_chart: null,
    draw_realtime_illuminance_chart: function() {
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
            yAxis : { title: {text : "照度(ルクス)"} },
            series: [
               {
                  name: fplugstatd.current_device,
                  data: fplugstatd.realtime_illuminance_values,
               }
            ]
        }
        fplugstatd.realtime_illuminance_chart = new Highcharts.Chart(options);
    },
    realtime_watt_chart: null,
    draw_realtime_watt_chart: function() {
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
            series: [
               {
                  name: fplugstatd.current_device,
                  data: fplugstatd.realtime_watt_values,
               }
            ]
        }
        fplugstatd.realtime_watt_chart = new Highcharts.Chart(options);
    },
    hourly_temperature_chart: null,
    draw_hourly_temperature_chart: function() {
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
            title : { text: "温度 (リアルタイム)" },
            xAxis : {
                title: {text : "24時間"},
                allowDecimals: false
            },
            yAxis : { title: {text : "温度(℃)"} },
            series: [
               {
                  name: fplugstatd.current_device,
                  data: fplugstatd.hourly_temperature_values,
               }
            ]
        }
        fplugstatd.hourly_temperature_chart = new Highcharts.Chart(options);
    },
    hourly_humidity_chart: null,
    draw_hourly_humidity_chart: function() {
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
            title : { text: "湿度 (リアルタイム)" },
            xAxis : {
                title: {text : "24時間"},
                allowDecimals: false
            },
            yAxis : {title: {text : "湿度(%)"}},
            series: [
               {
                  name: fplugstatd.current_device,
                  data: fplugstatd.hourly_humidity_values,
               }
            ]
        }
        fplugstatd.hourly_humidity_chart = new Highcharts.Chart(options);
    },
    hourly_illuminance_chart: null,
    draw_hourly_illuminance_chart: function() {
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
            title : { text: "照度 (リアルタイム)" },
            xAxis : {
                title: {text : "24時間"},
                allowDecimals: false
            },
            yAxis : {title: {text : "照度(ルクス)"}},
            series: [
               {
                  name: fplugstatd.current_device,
                  data: fplugstatd.hourly_illuminance_values,
               }
            ]
        }
        fplugstatd.hourly_illuminance_chart = new Highcharts.Chart(options);
    },
    hourly_watt_chart: null,
    draw_hourly_watt_chart: function() {
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
            title : { text: "電力 (リアルタイム)" },
            xAxis : {
                title: {text : "24時間"},
                allowDecimals: false
            },
            yAxis : {title: {text : "電力(W)"}},
            series: [
               {
                  name: fplugstatd.current_device,
                  data: fplugstatd.hourly_watt_values,
               }
            ]
        }
        fplugstatd.hourly_watt_chart = new Highcharts.Chart(options);
    }
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
    $("#body-fplug-realtime").tabs();
    $("#body-fplug-hourly").tabs();
    $("#sidebar-fplug-device").change(function() {
        fplugstatd.current_device = $("#sidebar-fplug-device").val();
        fplugstatd.get_realtime_values();
        fplugstatd.get_hourly_power_values();
        fplugstatd.get_hourly_other_values();
    });
    $("#sidebar-fplug-realtime").click(function(event) {
        fplugstatd.select_view("body-fplug-realtime", $(event.target).attr("id"));
        if (fplugstatd.hourly_interval != null) {
            clearInterval(fplugstatd.hourly_interval);
            fplugstatd.hourly_interval = null;
        }
        fplugstatd.get_realtime_values();
        fplugstatd.realtime_interval = setInterval(function(){
            fplugstatd.get_realtime_values();
        }, 1000 * 10);
	return false;
    });
    $("#sidebar-fplug-hourly").click(function(event) {
        fplugstatd.select_view("body-fplug-hourly", $(event.target).attr("id"));
        if (fplugstatd.realtime_interval != null) {
            clearInterval(fplugstatd.realtime_interval);
            fplugstatd.realtime_interval = null;
        }
        fplugstatd.get_hourly_power_values();
        fplugstatd.get_hourly_other_values();
        fplugstatd.hourly_interval = setInterval(function(){
            fplugstatd.get_hourly_power_values();
            fplugstatd.get_hourly_other_values();
        }, 1000 * 60);
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
    });
    $("#body-fplug-control-powerstart").click(function(event) {
        fplugstatd.start_hourly_power_store();
    });
    $("#body-fplug-control-datetimesetting").click(function(event) {
        fplugstatd.device_set_datetime();
    });
    fplugstatd.select_view("body-fplug-realtime", "sidebar-fplug-realtime");
    $.ajax({
        method: "GET",
        url: "/api/devicies",
        cache: false
    }).done(function(msg) {
        for (var i = 0; i < msg.length; i++) {
            $("#sidebar-fplug-device").append($("<option>").val(msg[i].address).text(msg[i].name));
        }
        fplugstatd.current_device = $("#sidebar-fplug-device").val();
        fplugstatd.get_realtime_values();
        fplugstatd.get_hourly_power_values();
        fplugstatd.get_hourly_other_values();
    }); 
});

