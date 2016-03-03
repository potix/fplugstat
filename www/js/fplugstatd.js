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
            fplugstatd.draw_realtime_watt_chart();
            fplugstatd.draw_realtime_illuminance_chart();
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
                fplugstatd.hourly_watt_values.push([elm.index, elm.watt]);
            }
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
                fplugstatd.hourly_temperature_values.push([elm.index, elm.temperature]);
                fplugstatd.hourly_humidity_values.push([elm.index, elm.humidity]);
                fplugstatd.hourly_illuminance_values.push([elm.index, elm.illuminance]);
            }
        });
    },
    realtime_temperature_chart: null,
    draw_realtime_temperature_chart: function() {
        var options = {
            chart : {renderTo : "body-fplug-realtime-temperature-chart"},
            rangeSelector : { selected : 1 },
            title : {text: "温度 (リアルタイム)"},
            xAxis : { type: "datetime",
                      title: {text : "時間"}},
            yAxis : {title: {text : "温度(℃)"}},
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
            chart : {renderTo : "body-fplug-realtime-humidity-chart"},
            rangeSelector : { selected : 1 },
            title : {text: "湿度 (リアルタイム)"},
            xAxis : { type: "datetime",
                      title: {text : "時間"}},
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
            chart : {renderTo : "body-fplug-realtime-illuminance-chart"},
            rangeSelector : { selected : 1 },
            title : {text: "照度 (リアルタイム)"},
            xAxis : { type: "datetime",
                      title: {text : "時間"}},
            yAxis : {title: {text : "照度(ルクス)"}},
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
            chart : {renderTo : "body-fplug-realtime-watt-chart"},
            rangeSelector : { selected : 1 },
            title : {text: "電力 (リアルタイム)"},
            xAxis : { type: "datetime",
                      title: {text : "時間"}},
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
    },
    hourly_humidity_chart: null,
    draw_houtly_humidity_chart: function() {
    },
    hourly_illuminance_chart: null,
    draw_hourly_illuminance_chart: function() {
    },
    hourly_watt_chart: null,
    draw_hourly_watt_chart: function() {
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
        fplugstatd.realtime_interval = setInterval(function(){
            fplugstatd.get_realtime_values();
        }, 1000 * 5);
	return false;
    });
    $("#sidebar-fplug-hourly").click(function(event) {
        fplugstatd.select_view("body-fplug-hourly", $(event.target).attr("id"));
        if (fplugstatd.realtime_interval != null) {
            clearInterval(fplugstatd.realtime_interval);
            fplugstatd.realtime_interval = null;
        }
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



// あとはjavascriptかく

/*

// view切り替えイベント (デフォルトはリアルタイム)
                        <li class="menu-child" id="sidebar-fplug-realtime" > リアルタイム情報</li>
                        <li class="menu-child" id="sidebar-fplug-hourly"> 時間毎の情報 </li>
                        <li class="menu-child" id="sidebar-fplug-realtime"> 制御 </li>



// chart 表示 1
                       <div id="body-fplug-realtime-temperature-fragment">
                           <div id="body-fplug-realtime-temperature-chart">
                               1
                           </div>
                       </div>
                       <div id="body-fplug-realtime-humidity-fragment">
                           <div id="body-fplug-realtime-humidity-chart">
                               2
                           </div>
                       </div>
                       <div id="body-fplug-realtime-illuminance-fragment">
                           <div id="body-fplug-realtime-illuminance-chart">
                               3
                           </div>
                       </div>
                       <div id="body-fplug-realtime-watt-fragment">
                           <div id="body-fplug-realtime-watt-chart">
                               4
                           </div>
                       </div>

                        
// chart 表示 1
                       <div id="body-fplug-hourly-temperature-fragment">
                           <div id="body-fplug-hourly-temperature-chart">
                               1
                           </div>
                       </div>
                       <div id="body-fplug-hourly-humidity-fragment">
                           <div id="body-fplug-realtime-humidity-chart">
                               2
                           </div>
                       </div>
                       <div id="body-fplug-hourly-illuminance-fragment">
                           <div id="body-fplug-hourly-illuminance-chart">
                               3
                           </div>
                       </div>
                       <div id="body-fplug-hourly-watt-fragment">
                           <div id="body-fplug-hourly-watt-chart">
                               4
                           </div>
                       </div>
// 制御イベント
                    <div id="body-fplug-control">
                        <button type="button" id="body-fplug-control-reset">リセット</button><br /><br />
                        <button type="button" id="body-fplug-control-powerstart">電力取得開始</button><br /><br />
                        <button type="button" id="body-fplug-control-datetimesetting">時刻設定</button><br /><br />
                    </div>

*/
