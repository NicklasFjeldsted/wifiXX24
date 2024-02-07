/**
 * @file app.js
 * @brief Initializes and updates a SciChart chart with real-time data via Server-Sent Events.
 */

/** 
 * @var {number} xValue 
 * @brief The current x-axis value for appending new data points to the chart.
 */
let xValue = 0;

/** 
 * @var {object} dataSeries 
 * @brief Holds the data series for the chart. Initialized by initSciChart function.
 */
let dataSeries;

/** 
 * @var {number} POINTS_LOOP 
 * @brief Defines the loop point for the x-axis value. When reached, xValue resets to 0.
 */
const POINTS_LOOP = 256;

/** 
 * @var {EventSource} evtSource 
 * @brief Establishes a connection to the server for receiving real-time data updates via Server-Sent Events.
 */
const evtSource = new EventSource('/events');

/**
 * @brief Listens for "value" events from the server and updates the chart with received data.
 */
evtSource.addEventListener("value", function(event) {
    const data = JSON.parse(event.data);
    if (dataSeries) {
        if (xValue >= POINTS_LOOP) {
            xValue = 0; // Reset xValue for looping effect
        }
        dataSeries.append(xValue, data.val); // Append new data point to the series
        xValue += 1; // Increment xValue for the next data point
    }
}, false);

/**
 * @brief Asynchronously initializes the SciChart environment, creates a chart, and configures its axes and series.
 */
async function initSciChart() {
    const { SciChartSurface, NumericAxis, XyDataSeries, FastLineRenderableSeries, EllipsePointMarker } = window.SciChart;
    const { sciChartSurface, wasmContext } = await SciChartSurface.create("scichart-root");
    
    // Configure x-axis
    const xAxis = new NumericAxis(wasmContext, {
        visibleRange: new SciChart.NumberRange(0, POINTS_LOOP),
        isVisible: false, // Hides the x-axis for a cleaner look
    });
    
    // Configure y-axis
    const yAxis = new NumericAxis(wasmContext, {
        visibleRange: new SciChart.NumberRange(0, 500),
        isVisible: false, // Hides the y-axis for a cleaner look
    });
    
    sciChartSurface.xAxes.add(xAxis);
    sciChartSurface.yAxes.add(yAxis);
    
    // Initialize data series with FIFO capacity
    dataSeries = new XyDataSeries(wasmContext, {
        fifoCapacity: POINTS_LOOP,
        fifoSweeping: true,
        fifoSweepingGap: 16
    });
    
    // Create and configure a line series
    const lineSeries = new FastLineRenderableSeries(wasmContext, {
        dataSeries: dataSeries,
        pointMarker: new EllipsePointMarker(wasmContext, {
            width: 11,   
            height: 11,
            fill: "#fff",
            lastPointOnly: true
        }),
        strokeThickness: 3,
        stroke: "#f48420"
    });
    
    sciChartSurface.renderableSeries.add(lineSeries);
}

// Call initSciChart to set up the chart
initSciChart();
