let temperatureChart = null;
let autoUpdateInterval = null;

function formatDateTimeLocal(d) {
    return d.toISOString().slice(0, 16);
}

function formatDateTimeForApi(d) {
    return d.toISOString().slice(0, 19) + 'Z';
}

function setDefaultTimeRange() {
    const to = new Date();
    const from = new Date(to.getTime() - 24 * 60 * 60 * 1000);

    document.getElementById('from').value = formatDateTimeLocal(from);
    document.getElementById('to').value = formatDateTimeLocal(to);
}

async function fetchCurrentData() {
    try {
        const response = await fetch('/api/current');
        if (!response.ok) throw new Error('Network error');

        const data = await response.json();

        if (data.data && data.data.length > 0) {
            const latest = data.data[0];
            const currentDiv = document.getElementById('current-data');
            const time = new Date(latest.timestamp);

            currentDiv.innerHTML = `
                <div style="margin-bottom: 10px;">
                    <span class="live-indicator"></span>
                    <strong>Режим реального времени</strong>
                </div>
                <div class="current-temp">${latest.temperature.toFixed(2)} °C</div>
                <div class="current-time">
                    ${time.toLocaleDateString('ru-RU')} ${time.toLocaleTimeString('ru-RU')}
                </div>
            `;
        }
    } catch (error) {
        console.error('Ошибка загрузки текущих данных:', error);
        document.getElementById('current-data').innerHTML =
            '<p style="color: #f5576c;">Ошибка загрузки данных</p>';
    }
}

async function loadChartData(from, to) {
    try {
        const fromStr = encodeURIComponent(formatDateTimeForApi(from));
        const toStr = encodeURIComponent(formatDateTimeForApi(to));

        const response = await fetch(`/api/range?from=${fromStr}&to=${toStr}`);
        if (!response.ok) throw new Error('Network error');

        const data = await response.json();

        if (!data.data || data.data.length === 0) {
            alert('Нет данных за выбранный период');
            return;
        }

        const timestamps = data.data.map(m => new Date(m.timestamp));
        const temperatures = data.data.map(m => m.temperature);

        if (!temperatureChart) {
            const ctx = document.getElementById('temperatureChart').getContext('2d');
            temperatureChart = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: timestamps,
                    datasets: [{
                        label: 'Температура (°C)',
                        data: temperatures,
                        borderColor: 'rgb(75, 192, 192)',
                        backgroundColor: 'rgba(75, 192, 192, 0.1)',
                        tension: 0.3,
                        fill: true,
                        pointRadius: 2,
                        borderWidth: 2
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    scales: {
                        x: {
                            type: 'time',
                            time: {
                                unit: 'hour',
                                displayFormats: {
                                    hour: 'dd.MM HH:mm'
                                }
                            },
                            title: {
                                display: true,
                                text: 'Время'
                            }
                        },
                        y: {
                            title: {
                                display: true,
                                text: 'Температура (°C)'
                            },
                            ticks: {
                                callback: function(value) {
                                    return value + '°C';
                                }
                            }
                        }
                    },
                    plugins: {
                        legend: {
                            display: true,
                            position: 'top'
                        },
                        tooltip: {
                            mode: 'index',
                            intersect: false,
                            callbacks: {
                                label: function(context) {
                                    return `Температура: ${context.parsed.y.toFixed(2)}°C`;
                                }
                            }
                        }
                    }
                }
            });
        } else {
            temperatureChart.data.labels = timestamps;
            temperatureChart.data.datasets[0].data = temperatures;
            temperatureChart.update();
        }
    } catch (error) {
        console.error('Ошибка загрузки данных графика:', error);
        alert('Ошибка загрузки данных: ' + error.message);
    }
}

async function loadStatistics(from, to) {
    try {
        const fromStr = encodeURIComponent(formatDateTimeForApi(from));
        const toStr = encodeURIComponent(formatDateTimeForApi(to));

        const response = await fetch(`/api/stats?from=${fromStr}&to=${toStr}`);
        if (!response.ok) throw new Error('Network error');

        const stats = await response.json();

        const statsDiv = document.getElementById('stats');
        statsDiv.innerHTML = `
            <div class="stat-card">
                <h4>📊 Количество измерений</h4>
                <div class="stat-value">${stats.count}</div>
            </div>
            <div class="stat-card">
                <h4>📈 Средняя температура</h4>
                <div class="stat-value">${stats.average.toFixed(2)}°C</div>
            </div>
            <div class="stat-card">
                <h4>🔥 Максимальная</h4>
                <div class="stat-value">${stats.max.toFixed(2)}°C</div>
            </div>
            <div class="stat-card">
                <h4>❄️ Минимальная</h4>
                <div class="stat-value">${stats.min.toFixed(2)}°C</div>
            </div>
        `;
    } catch (error) {
        console.error('Ошибка загрузки статистики:', error);
    }
}

async function loadData() {
    const fromInput = document.getElementById('from').value;
    const toInput = document.getElementById('to').value;

    if (!fromInput || !toInput) {
        alert('Пожалуйста, укажите период');
        return;
    }

    const from = new Date(fromInput);
    const to = new Date(toInput);

    if (from >= to) {
        alert('Начальная дата должна быть меньше конечной');
        return;
    }

    await loadChartData(from, to);
    await loadStatistics(from, to);
}

function loadLastHour() {
    const to = new Date();
    const from = new Date(to.getTime() - 60 * 60 * 1000);
    document.getElementById('from').value = formatDateTimeLocal(from);
    document.getElementById('to').value = formatDateTimeLocal(to);
    loadData();
}

function loadLastDay() {
    const to = new Date();
    const from = new Date(to.getTime() - 24 * 60 * 60 * 1000);
    document.getElementById('from').value = formatDateTimeLocal(from);
    document.getElementById('to').value = formatDateTimeLocal(to);
    loadData();
}

function loadLastWeek() {
    const to = new Date();
    const from = new Date(to.getTime() - 7 * 24 * 60 * 60 * 1000);
    document.getElementById('from').value = formatDateTimeLocal(from);
    document.getElementById('to').value = formatDateTimeLocal(to);
    loadData();
}

function startAutoUpdate() {
    // Обновляем текущие данные каждые 10 секунд
    autoUpdateInterval = setInterval(fetchCurrentData, 10000);
}

// Инициализация при загрузке страницы
window.addEventListener('DOMContentLoaded', function() {
    setDefaultTimeRange();
    loadData();
    fetchCurrentData();
    startAutoUpdate();
});

// Очистка интервала при закрытии страницы
window.addEventListener('beforeunload', function() {
    if (autoUpdateInterval) {
        clearInterval(autoUpdateInterval);
    }
});