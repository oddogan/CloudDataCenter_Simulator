function data = ParseData(filename)
    % File size
    s = dir(filename);         
    filesize = s.bytes;

    % Define the number of variables per record
    bytesPerRecord = 6 * 8 + 8 + 8 + 8 + 8; % 6 doubles (8 bytes each) + 1 uint64 (8 bytes) + 2 double + 1 uint64
    
    % Compute the total number of records
    totalRecords = floor(filesize / bytesPerRecord);
    
    % Memory-map the file
    m = memmapfile(filename, 'Format', 'uint8', 'Writable', false);
    
    % Extract raw bytes as a single vector
    rawData = m.Data(1:totalRecords * bytesPerRecord);
    % Preallocate structured output
    data = struct();
    data.time = zeros(1, totalRecords);
    data.cpu = zeros(1, totalRecords);
    data.ram = zeros(1, totalRecords);
    data.disk = zeros(1, totalRecords);
    data.bandwidth = zeros(1, totalRecords);
    data.fpga = zeros(1, totalRecords);
    data.turnedOnMachineCount = zeros(1, totalRecords, 'uint64');
    data.averagePowerConsumption = zeros(1, totalRecords);
    data.totalPowerConsumption = zeros(1, totalRecords);
    data.numberOfSLAVs = zeros(1, totalRecords, 'uint64');

    % **Extract interleaved data correctly using strided access**
    for i = 1:totalRecords
        startIdx = (i-1) * bytesPerRecord + 1;
        
        data.time(i) = typecast(rawData(startIdx : startIdx + 7), 'double');
        startIdx = startIdx + 8;
        
        data.cpu(i) = typecast(rawData(startIdx : startIdx + 7), 'double');
        startIdx = startIdx + 8;
        
        data.ram(i) = typecast(rawData(startIdx : startIdx + 7), 'double');
        startIdx = startIdx + 8;
        
        data.disk(i) = typecast(rawData(startIdx : startIdx + 7), 'double');
        startIdx = startIdx + 8;
        
        data.bandwidth(i) = typecast(rawData(startIdx : startIdx + 7), 'double');
        startIdx = startIdx + 8;
        
        data.fpga(i) = typecast(rawData(startIdx : startIdx + 7), 'double');
        startIdx = startIdx + 8;
        
        data.turnedOnMachineCount(i) = typecast(rawData(startIdx : startIdx + 7), 'uint64');
        startIdx = startIdx + 8;
        
        data.averagePowerConsumption(i) = typecast(rawData(startIdx : startIdx + 7), 'double');
        startIdx = startIdx + 8;
        
        data.totalPowerConsumption(i) = typecast(rawData(startIdx : startIdx + 7), 'double');
        startIdx = startIdx + 8;
        
        data.numberOfSLAVs(i) = typecast(rawData(startIdx : startIdx + 7), 'uint64');
    end
end