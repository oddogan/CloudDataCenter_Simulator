function PlotDataOptimized(varargin)
    figure;
    set(gcf, 'Units', 'Normalized', 'OuterPosition', [0 0 1 1]);
    
    colors = lines(length(varargin)); % Generate distinguishable colors

    % Check if names are provided
    dataSets = varargin;
    names = arrayfun(@(x) sprintf('Data %d', x), 1:length(varargin), 'UniformOutput', false);
    if mod(length(varargin), 2) == 0
        dataSets = varargin(1:2:end);
        names = varargin(2:2:end);
    end

    % Downsampling factor (adjust as needed)
    ds_factor = 1; % Plot every 100th point

    % Helper function for downsampling
    downsampleData = @(x) x(1:ds_factor:end);

    % Plot global utilizations on the same plot
    subplot(3,2,1);
    hold on;
    for i = 1:length(dataSets)
        data = dataSets{i};
        color = colors(i, :);
        plot(downsampleData(data.time), downsampleData(data.cpu), '-', 'Color', color, 'LineWidth', 1.5, 'DisplayName', sprintf('%s - CPU', names{i}));
        plot(downsampleData(data.time), downsampleData(data.ram), ':', 'Color', color, 'LineWidth', 1.5, 'DisplayName', sprintf('%s - RAM', names{i}));
        % plot(downsampleData(data.time), downsampleData(data.disk), ':', 'Color', color, 'LineWidth', 1.5, 'DisplayName', sprintf('%s - Disk', names{i}));
        % plot(downsampleData(data.time), downsampleData(data.bandwidth), '-.', 'Color', color, 'LineWidth', 1.5, 'DisplayName', sprintf('%s - Bandwidth', names{i}));
    end
    hold off;
    xlabel('Örnek'); ylabel('Kullanım (%)');
    title('Ortalama Kaynak Kullanımı');
    legend('Location', 'best'); grid on;
    drawnow limitrate; % Reduce figure updates for performance

    % Plot turned on machine count
    subplot(3,2,3);
    hold on;
    for i = 1:length(dataSets)
        data = dataSets{i};
        color = colors(i, :);
        plot(downsampleData(data.time), downsampleData(data.turnedOnMachineCount), '-', 'Color', color, 'LineWidth', 1.5, 'DisplayName', sprintf('%s', names{i}));
    end
    hold off;
    xlabel('Örnek'); ylabel('Sayı');
    title('Açık Fiziksel Makine Sayısı');
    legend('Location', 'best'); grid on;
    drawnow limitrate;

    % SLA Violations
    subplot(3,2,4);
    hold on;
    for i = 1:length(dataSets)
        data = dataSets{i};
        color = colors(i, :);
        plot(downsampleData(data.time), downsampleData(data.numberOfSLAVs), '-', 'Color', color, 'LineWidth', 1.5, 'DisplayName', sprintf('%s', names{i}));
    end
    hold off;
    xlabel('Örnek'); ylabel('Sayı');
    title('# SLA İhlali');
    legend('Location', 'best'); grid on;
    drawnow limitrate;

    % Average Power Consumption
    subplot(3,2,5);
    hold on;
    for i = 1:length(dataSets)
        data = dataSets{i};
        color = colors(i, :);
        plot(downsampleData(data.time), downsampleData(data.averagePowerConsumption), '-', 'Color', color, 'LineWidth', 1.5, 'DisplayName', sprintf('%s', names{i}));
    end
    hold off;
    xlabel('Örnek'); ylabel('Güç Tüketimi (W)');
    title('Ortalama Güç Tüketimi');
    legend('Location', 'best'); grid on;
    drawnow limitrate;

    % Total Power Consumption (scaled to kW)
    subplot(3,2,6);
    hold on;
    for i = 1:length(dataSets)
        data = dataSets{i};
        color = colors(i, :);
        plot(downsampleData(data.time), downsampleData(data.totalPowerConsumption / 1e3), '-', 'Color', color, 'LineWidth', 1.5, 'DisplayName', sprintf('%s', names{i}));
    end
    hold off;
    xlabel('Örnek'); ylabel('Güç Tüketimi (kW)');
    title('Toplam Güç Tüketimi');
    legend('Location', 'best'); grid on;
    drawnow limitrate;
    fontsize(gcf,scale=1.2)
end