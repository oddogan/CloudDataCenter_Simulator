function data = ParseData(filename)
    % Define the number of variables per record
    bytesPerRecord = 6 * 8 + 8 + 8 + 8 + 8; % 6 doubles (8 bytes each) + 1 uint64 (8 bytes) + 2 double + 1 uint64
    
    % Map the file
    m = memmapfile(filename, 'Format', 'uint8', 'Writable', false);
    
    % Compute the total number of records
    totalRecords = floor(numel(m.Data) / bytesPerRecord);
    
    % Reshape raw data into a matrix
    rawData = reshape(m.Data(1:totalRecords * bytesPerRecord), bytesPerRecord, [])';
    
    % Convert bytes to usable data
    data = struct();
    data.time = typecast(reshape(rawData(:, 1:8)', [], 1), 'double')';
    data.cpu = typecast(reshape(rawData(:, 9:16)', [], 1), 'double')';
    data.ram = typecast(reshape(rawData(:, 17:24)', [], 1), 'double')';
    data.disk = typecast(reshape(rawData(:, 25:32)', [], 1), 'double')';
    data.bandwidth = typecast(reshape(rawData(:, 33:40)', [], 1), 'double')';
    data.fpga = typecast(reshape(rawData(:, 41:48)', [], 1), 'double')';
    data.turnedOnMachineCount = typecast(reshape(rawData(:, 49:56)', [], 1), 'uint64')';
    data.averagePowerConsumption = typecast(reshape(rawData(:, 57:64)', [], 1), 'double')';
    data.totalPowerConsumption = typecast(reshape(rawData(:, 65:72)', [], 1), 'double')';
    data.numberOfSLAVs = typecast(reshape(rawData(:, 73:80)', [], 1), 'uint64')';
end