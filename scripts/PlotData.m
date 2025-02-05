function PlotData(varargin)
    figure;
    
    colors = lines(length(varargin)); % Generate distinguishable colors

    % Check if names are provided
    dataSets = varargin;
    names = arrayfun(@(x) sprintf('Data %d', x), 1:length(varargin), 'UniformOutput', false);
    if mod(length(varargin), 2) == 0
        dataSets = varargin(1:2:end);
        names = varargin(2:2:end);
    end

    % Plot global utilizations on the same plot
    subplot(2,1,1);
    hold on;
    for i = 1:length(dataSets)
        data = dataSets{i};
        color = colors(i, :);
        plot(data.time, data.cpu, '-', 'Color', color, 'LineWidth', 1.5, 'DisplayName', sprintf('%s - CPU', names{i}));
        plot(data.time, data.ram, '--', 'Color', color, 'LineWidth', 1.5, 'DisplayName', sprintf('%s - RAM', names{i}));
        plot(data.time, data.disk, ':', 'Color', color, 'LineWidth', 1.5, 'DisplayName', sprintf('%s - Disk', names{i}));
        plot(data.time, data.bandwidth, '-.', 'Color', color, 'LineWidth', 1.5, 'DisplayName', sprintf('%s - Bandwidth', names{i}));
        plot(data.time, data.fpga, '--', 'Color', color, 'LineWidth', 1.5, 'DisplayName', sprintf('%s - FPGA', names{i}));
    end
    hold off;
    xlabel('Time');
    ylabel('Utilization');
    title('Global Resource Utilization');
    legend;
    grid on;
    
    % Plot turned on machine count
    subplot(2,2,3);
    hold on;
    for i = 1:length(dataSets)
        data = dataSets{i};
        color = colors(i, :);
        plot(data.time, data.turnedOnMachineCount, '-', 'Color', color, 'LineWidth', 1, 'DisplayName', sprintf('%s - Machines', names{i}));
    end
    hold off;
    xlabel('Time');
    ylabel('Count');
    title('Turned On Machine Count');
    legend;
    grid on;
    
    subplot(2,2,4);
    hold on;
    for i = 1:length(dataSets)
        data = dataSets{i};
        color = colors(i, :);
        plot(data.time, data.averagePowerConsumption, '-', 'Color', color, 'LineWidth', 0.8, 'DisplayName', sprintf('%s - Power', names{i}));
    end
    hold off;
    xlabel('Time');
    ylabel('Power Consumption');
    title('Average Power Consumption');
    legend;
    grid on;
end