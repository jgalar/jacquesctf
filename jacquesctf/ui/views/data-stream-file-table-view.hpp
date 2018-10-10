/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_DATA_STREAM_FILE_TABLE_VIEW_HPP
#define _JACQUES_DATA_STREAM_FILE_TABLE_VIEW_HPP

#include "state.hpp"
#include "data-size.hpp"
#include "timestamp.hpp"
#include "table-view.hpp"

namespace jacques {

class DataStreamFileTableView :
    public TableView
{
public:
    explicit DataStreamFileTableView(const Rectangle& rect,
                                     std::shared_ptr<const Stylist> stylist,
                                     std::shared_ptr<State> state);
    Index selectedDataStreamFileIndex() const;
    void selectedDataStreamFileIndex(Index index);
    void timestampFormatMode(TimestampFormatMode tsFormatMode);
    void dataSizeFormatMode(utils::SizeFormatMode dataSizeFormatMode);

private:
    void _drawRow(Index index) override;
    bool _hasIndex(Index index) override;
    void _selectLast() override;
    void _resized() override;
    void _stateChanged(const Message& msg) override;
    void _setColumnDescriptions();
    void _resetRow(const std::vector<TableViewColumnDescription>& descrs);

private:
    std::vector<std::unique_ptr<TableViewCell>> _row;
    std::shared_ptr<State> _state;
    const ViewStateObserverGuard _stateObserverGuard;
};

} // namespace jacques

#endif // _JACQUES_DATA_STREAM_FILE_TABLE_VIEW_HPP
